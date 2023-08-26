/*
    Copyright 2023 Jesse Talavera-Greenberg

    melonDS DS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS DS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS DS. If not, see http://www.gnu.org/licenses/.
*/

#include "environment.hpp"
#include "environment.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <libretro.h>
#include <file/file_path.h>
#include <streams/file_stream.h>
#include <compat/strl.h>
#include <retro_dirent.h>

#include "microphone.hpp"
#include "info.hpp"
#include "libretro.hpp"
#include "config.hpp"
#include "tracy.hpp"

using std::string;
using std::string_view;
using std::optional;
using std::nullopt;
using std::vector;
using namespace std::literals;

namespace retro {
    static retro_environment_t _environment;
    static retro_video_refresh_t _video_refresh;
    static retro_audio_sample_batch_t _audio_sample_batch;
    static retro_input_poll_t _input_poll;
    static retro_input_state_t _input_state;
    static retro_log_printf_t _log;
    static bool _supports_bitmasks;
    static bool _supportsPowerStatus;
    static bool isShuttingDown = false;
    static unsigned _message_interface_version;

    // Cached so that the save directory won't change during a session
    static optional<string> _save_directory;
    static optional<string> _system_directory;
    static optional<string> _system_subdir;
    static optional<string> _system_fallback_subdir;

    static void log(enum retro_log_level level, const char* fmt, va_list va) noexcept;
}

bool retro::environment(unsigned cmd, void* data) noexcept {
    if (_environment) {
        return _environment(cmd, data);
    } else {
        return false;
    }
}

int16_t retro::input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
    ZoneScopedN("retro::input_state");
    if (_input_state) {
        return _input_state(port, device, index, id);
    } else {
        return 0;
    }
}

void retro::input_poll() {
    ZoneScopedN("retro::input_poll");
    if (_input_poll) {
        _input_poll();
    }
}

size_t retro::audio_sample_batch(const int16_t* data, size_t frames) {
    ZoneScopedN("retro::audio_sample_batch");
    if (_audio_sample_batch) {
        return _audio_sample_batch(data, frames);
    } else {
        return 0;
    }
}

void retro::video_refresh(const void* data, unsigned width, unsigned height, size_t pitch) {
    ZoneScopedN("retro::video_refresh");
    if (_video_refresh) {
        _video_refresh(data, width, height, pitch);
    }
}

bool retro::set_screen_rotation(ScreenOrientation orientation) noexcept {
    bool rotated = false;
    rotated = environment(RETRO_ENVIRONMENT_SET_ROTATION, &orientation);
    return rotated;
}

// reminder: std::string_views are NOT null-terminated!
// Even if they're created from null-terminated strings, the null byte is outside the view
bool retro::set_core_options(const retro_core_options_v2& options) noexcept {
    unsigned version = 0;
    if (!retro::environment(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
        version = 0;

    if (version >= 2) {
        if (retro::environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, (void *) &options)) {
            retro::debug("V2 core options set successfully");
            return true;
        }
    }

    retro::warn("V2 core options not supported, trying V1");

    unsigned num_options = 0;
    while (true) {
        if (options.definitions[num_options].key)
            num_options++;
        else
            break;
    }

    if (version >= 1) {
        /* Allocate US array */
        vector<retro_core_option_definition> optionDefsV1(num_options + 1);

        /* Copy parameters from option_defs_us array */
        for (unsigned i = 0; i < num_options; i++) {
            retro_core_option_v2_definition& optionDefV2 = options.definitions[i];
            retro_core_option_definition& optionDefV1 = optionDefsV1[i];

            optionDefV1.key = optionDefV2.key;
            optionDefV1.desc = optionDefV2.desc;
            optionDefV1.info = optionDefV2.info;
            optionDefV1.default_value = optionDefV2.default_value;
            memcpy(optionDefV1.values, optionDefV2.values, sizeof(optionDefV1.values));
        }

        memset(&optionDefsV1.back(), 0, sizeof(retro_core_option_definition));

        if (retro::environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, optionDefsV1.data())) {
            retro::debug("V1 core options set successfully");
            return true;
        }
    }

    retro::warn("V1 core options not supported, trying V0");

    /* Allocate arrays */
    vector<retro_variable> variables(num_options + 1);
    vector<string> valuesBuffer(num_options);

    size_t option_index = 0;
    /* Copy parameters from option_defs_us array */
    for (unsigned i = 0; i < num_options; i++) {
        string_view desc = options.definitions[i].desc ? options.definitions[i].desc : ""sv;
        string_view default_value = options.definitions[i].default_value ? options.definitions[i].default_value : ""sv;
        retro_core_option_value* values = options.definitions[i].values;
        size_t default_index = 0;

        valuesBuffer[i] = "";

        if (!desc.empty()) {
            size_t num_values = 0;

            /* Determine number of values */
            while (true) {
                if (values[num_values].value) {
                    /* Check if this is the default value */
                    if (!default_value.empty() && values[num_values].value == default_value)
                        default_index = num_values;

                    num_values++;
                } else
                    break;
            }

            /* Build values string */
            if (num_values > 0) {

                /* Default value goes first */
                valuesBuffer[i] = string(desc) + "; " + values[default_index].value;

                /* Add remaining values */
                for (unsigned j = 0; j < num_values; j++) {
                    if (j != default_index) {
                        valuesBuffer[i] += string("|") + values[j].value;
                    }
                }
            }
        }

        variables[option_index].key = options.definitions[i].key;
        variables[option_index].value = valuesBuffer[i].c_str();
        option_index++;
    }

    memset(&variables.back(), 0, sizeof(retro_variable));

    /* Set variables */
    return retro::environment(RETRO_ENVIRONMENT_SET_VARIABLES, variables.data());
}

bool retro::shutdown() noexcept {
    if (melonds::IsInDeinit() || melonds::IsUnloadingGame())
        return true;

    if (isShuttingDown)
        return true;

    isShuttingDown = environment(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
    return isShuttingDown;
}

bool retro::is_variable_updated() noexcept {
    bool updated = false;
    environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);
    return updated;
}

void retro::log(enum retro_log_level level, const char* fmt, ...) noexcept {
    if (fmt == nullptr)
        return;

    va_list va;
    va_start(va, fmt);
    vlog(level, fmt, va);
    va_end(va);
}

void retro::debug(const char* fmt, ...) noexcept {
    if (fmt == nullptr)
        return;

    va_list va;
    va_start(va, fmt);
    vlog(RETRO_LOG_DEBUG, fmt, va);
    va_end(va);
}

void retro::info(const char* fmt, ...) noexcept {
    if (fmt == nullptr)
        return;

    va_list va;
    va_start(va, fmt);
    vlog(RETRO_LOG_INFO, fmt, va);
    va_end(va);
}

void retro::warn(const char* fmt, ...) noexcept {
    if (fmt == nullptr)
        return;

    va_list va;
    va_start(va, fmt);
    vlog(RETRO_LOG_WARN, fmt, va);
    va_end(va);
}

void retro::error(const char* fmt, ...) noexcept {
    if (fmt == nullptr)
        return;

    va_list va;
    va_start(va, fmt);
    vlog(RETRO_LOG_ERROR, fmt, va);
    va_end(va);
}

void retro_vlog(enum retro_log_level level, const char* fmt, va_list va) {
    retro::vlog(level, fmt, va);
}

void retro::vlog(enum retro_log_level level, const char* fmt, va_list va) noexcept {
    if (fmt == nullptr)
        return;

    if (_log) {
        // We can't pass the va_list directly to the libretro callback,
        // so we have to construct the string and print that
        char text[1024];
        memset(text, 0, sizeof(text));
        vsnprintf(text, sizeof(text), fmt, va);
        size_t text_length = strlen(text);
        if (text[text_length - 1] == '\n')
            text[text_length - 1] = '\0';

        _log(level, "%s\n", text);
    } else {
        vfprintf(stderr, fmt, va);
    }
}

bool retro::set_error_message(const char* message, unsigned duration) {
    if (message == nullptr) {
        log(RETRO_LOG_ERROR, "set_error_message: message is null");
        return false;
    }

    if (duration == 0) {
        log(RETRO_LOG_ERROR, "set_error_message: duration is 0");
        return false;
    }

    struct retro_message_ext message_ext {
        .msg = message,
        .duration = duration,
        .priority = retro::DEFAULT_ERROR_PRIORITY,
        .level = RETRO_LOG_ERROR,
        .target = RETRO_MESSAGE_TARGET_ALL,
        .type = RETRO_MESSAGE_TYPE_NOTIFICATION,
        .progress = -1
    };

    return set_message(&message_ext);
}

bool retro::set_warn_message(const char* message, unsigned duration) {
    if (message == nullptr) {
        log(RETRO_LOG_ERROR, "set_warn_message: message is null");
        return false;
    }

    if (duration == 0) {
        log(RETRO_LOG_ERROR, "set_warn_message: duration is 0");
        return false;
    }

    struct retro_message_ext message_ext {
        .msg = message,
        .duration = duration,
        .priority = retro::DEFAULT_ERROR_PRIORITY,
        .level = RETRO_LOG_WARN,
        .target = RETRO_MESSAGE_TARGET_ALL,
        .type = RETRO_MESSAGE_TYPE_NOTIFICATION,
        .progress = -1
    };

    return set_message(&message_ext);
}

bool retro::set_error_message(const char* message) {
    return set_error_message(message, retro::DEFAULT_ERROR_DURATION);
}

bool retro::set_warn_message(const char* message) {
    return set_warn_message(message, retro::DEFAULT_ERROR_DURATION);
}

bool retro::set_message(const struct retro_message_ext* message) {
    using std::numeric_limits;

    if (message == nullptr)
        return false;

    switch (_message_interface_version) {
        // Given that the frontend supports...
        case 0: { // ...the basic messaging interface...
            // Let's match the semantics of RETRO_ENVIRONMENT_SET_MESSAGE, since that's all we have
            float target_refresh_rate = 60.0f; // In FPS
            environment(RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE, &target_refresh_rate);

            struct retro_message msg {
                .msg = message->msg,
                // convert from ms to frames
                .frames = static_cast<unsigned int>((message->duration / 1000) * target_refresh_rate)
            };

            return environment(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
        }
        default:
            // ...a newer interface than we know about...
            // intentional fall-through
        case 1: { // ...the extended messaging interface...
            return environment(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, (void*) message);
        }
    }
}

bool retro::supports_bitmasks() {
    return _supports_bitmasks;
}

bool retro::get_variable(struct retro_variable* var) {
    return environment(RETRO_ENVIRONMENT_GET_VARIABLE, var);
}

const char* retro::get_variable(const char* key) {
    struct retro_variable var = {key, nullptr};
    if (!environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
        // Get the requested variable. If that failed...
        return nullptr;
    }

    if (key) {
        // If we wanted a specific variable...
        return var.value;
    }

    // Return the environment string instead
    return var.key;

}

bool retro::set_variable(const char* key, const char* value) {
    if (!key) {
        return false;
    }

    struct retro_variable var = {key, value};
    return environment(RETRO_ENVIRONMENT_SET_VARIABLE, &var);
}

optional<retro_language> retro::get_language() noexcept {
    retro_language language;
    if (!environment(RETRO_ENVIRONMENT_GET_LANGUAGE, &language)) {
        return nullopt;
    }

    if (language < 0 || language >= RETRO_LANGUAGE_LAST) {
        return nullopt;
    }

    return language;
}

optional<string> retro::username() noexcept {
    const char* username = nullptr;
    if (!environment(RETRO_ENVIRONMENT_GET_USERNAME, &username)) {
        return nullopt;
    }

    if (!username) {
        return nullopt;
    }

    return username;
}

void retro::set_option_visible(const char* key, bool visible) noexcept
{
    struct retro_core_option_display optionDisplay { key, visible };
    if (key) {
        environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &optionDisplay);
    }
}

bool retro::supports_power_status() noexcept {
    return _supportsPowerStatus;
}

optional<retro_device_power> retro::get_device_power() noexcept
{
    struct retro_device_power power;
    if (!environment(RETRO_ENVIRONMENT_GET_DEVICE_POWER, &power)) {
        return nullopt;
    }

    return power;
}

const optional<string>& retro::get_save_directory() {
    return _save_directory;
}

const optional<string>& retro::get_system_directory() {
    return _system_directory;
}

optional<string> retro::get_system_path(const string_view& name) noexcept {
    optional<string> sysdir = retro::get_system_directory();
    if (!sysdir) {
        // If no system directory is available, or the name is empty or not null-terminated...
        return nullopt;
    }

    char fullpath[PATH_MAX];
    size_t pathLength = fill_pathname_join_special(fullpath, sysdir->c_str(), name.data(), sizeof(fullpath));

    if (pathLength >= sizeof(fullpath)) {
        // If the path is too long...
        return nullopt;
    }

    pathname_make_slashes_portable(fullpath);
    return string(fullpath);
}

const optional<string>& retro::get_system_subdirectory() {
    return _system_subdir;
}

const optional<string>& retro::get_system_fallback_subdirectory() {
    return _system_fallback_subdir;
}

void retro::clear_environment() {
    _save_directory = nullopt;
    _system_directory = nullopt;
    _system_subdir = nullopt;
    microphone::clear_interface();
}

// This function might be called multiple times by the frontend,
// and not always with the same value of cb.
PUBLIC_SYMBOL void retro_set_environment(retro_environment_t cb) {
    ZoneScopedN("retro_set_environment");
    using retro::environment;
    retro::_environment = cb;

    // TODO: Handle potential errors with each environment call below
    struct retro_core_options_update_display_callback update_display_cb{melonds::update_option_visibility};
    environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK, &update_display_cb);

    environment(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE, (void*) melonds::content_overrides);
    environment(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*) melonds::ports);

    bool yes = true;
    environment(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &yes);

    retro_log_callback log_callback = {nullptr};
    if (environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log_callback)) {
        retro::_log = log_callback.log;
        retro::log(RETRO_LOG_DEBUG, "retro_set_environment(%p)", cb);
    }

    retro::_supports_bitmasks |= environment(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, nullptr);
    retro::_supportsPowerStatus |= environment(RETRO_ENVIRONMENT_GET_DEVICE_POWER, nullptr);

    if (retro::_supportsPowerStatus) {
        retro::debug("Power state available\n");
    }

    environment(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION, &retro::_message_interface_version);

    const char* save_dir = nullptr;
    if (retro::environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir) {
        retro::log(RETRO_LOG_INFO, "Save directory: \"%s\"", save_dir);
        retro::_save_directory = save_dir;
    }

    const char* system_dir = nullptr;
    if (retro::environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir) {
        char melon_dir[PATH_MAX];
        strlcpy(melon_dir, system_dir, sizeof(melon_dir));
        pathname_make_slashes_portable(melon_dir);
        retro::log(RETRO_LOG_INFO, "System directory: \"%s\"", &melon_dir);
        retro::_system_directory = melon_dir;

        memset(melon_dir, 0, sizeof(melon_dir));
        strlcpy(melon_dir, system_dir, sizeof(melon_dir));

        fill_pathname_join_special(melon_dir, system_dir, MELONDSDS_NAME, sizeof(melon_dir));
        pathname_make_slashes_portable(melon_dir);
        retro::_system_subdir = melon_dir;

        if (path_mkdir(melon_dir)) {
            retro::info("melonDS DS system directory: \"%s\"", melon_dir);
        }
        else {
            retro::error("Failed to create melonDS DS system subdirectory at \"%s\"", melon_dir);
        }

        fill_pathname_join_special(melon_dir, system_dir, "melonDS", sizeof(melon_dir));
        pathname_make_slashes_portable(melon_dir);
        retro::_system_fallback_subdir = melon_dir;

        retro::info("melonDS DS system fallback directory: \"%s\" (won't be created, but will be honored)", melon_dir);
    }

    environment(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void*) melonds::subsystems);

    retro_vfs_interface_info vfs { .required_interface_version = 1, .iface = nullptr };
    if (environment(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs)) {
        if (vfs.required_interface_version >= PATH_REQUIRED_VFS_VERSION) {
            path_vfs_init(&vfs);
        }

        if (vfs.required_interface_version >= FILESTREAM_REQUIRED_VFS_VERSION) {
            filestream_vfs_init(&vfs);
        }

        if (vfs.required_interface_version >= DIRENT_REQUIRED_VFS_VERSION) {
            dirent_vfs_init(&vfs);
        }
    }

    bool supports_no_game = true;
    if (environment(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &supports_no_game))
        retro::log(RETRO_LOG_INFO, "Frontend supports no-game mode.\n");

    retro::microphone::init_interface();
}

PUBLIC_SYMBOL void retro_set_video_refresh(retro_video_refresh_t video_refresh) {
    retro::_video_refresh = video_refresh;
}

PUBLIC_SYMBOL void retro_set_audio_sample(retro_audio_sample_t) {
    // Noop, we don't use this callback
}

PUBLIC_SYMBOL void retro_set_audio_sample_batch(retro_audio_sample_batch_t audio_sample_batch) {
    retro::_audio_sample_batch = audio_sample_batch;
}

PUBLIC_SYMBOL void retro_set_input_poll(retro_input_poll_t input_poll) {
    retro::_input_poll = input_poll;
}

PUBLIC_SYMBOL void retro_set_input_state(retro_input_state_t input_state) {
    retro::_input_state = input_state;
}