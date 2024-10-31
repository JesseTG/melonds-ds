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

#include <fmt/core.h>
#include <libretro.h>
#include <file/file_path.h>
#include <streams/file_stream.h>
#include <compat/strl.h>
#include <retro_dirent.h>
#include <retro_assert.h>
#include <string/stdstring.h>

#include "core/core.hpp"
#include "microphone.hpp"
#include "info.hpp"
#include "libretro.hpp"
#include "config/config.hpp"
#include "core/test.hpp"
#include "tracy.hpp"
#include "version.hpp"

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
    static bool _supportsNoGameMode;
    static bool isShuttingDown = false;
    static std::optional<std::chrono::microseconds> _lastFrameTime = std::nullopt;

    static unsigned _message_interface_version = UINT_MAX;
    constexpr size_t PATH_LENGTH = PATH_MAX + 1;
    constexpr string_view SUBDIR_SUFFIX = "/" MELONDSDS_NAME;
    // These paths are cached so that the save directory won't change during a session.
    // They're stored here as char arrays so that we don't have global heap memory.
    static char _saveDir[PATH_LENGTH] {};
    static size_t _saveDirLength = 0;

    static char _saveSubdir[PATH_LENGTH] {};
    static size_t _saveSubdirLength = 0;

    static char _sysDir[PATH_LENGTH] {};
    static size_t _sysDirLength = 0;

    static char _sysSubdir[PATH_LENGTH] {};
    static size_t _sysSubdirLength = 0;

    static retro_rumble_interface _rumble {};
    static retro_sensor_interface _sensor {};

    static void log(enum retro_log_level level, const char* fmt, va_list va) noexcept;
    static void NormalizePath(std::span<char> buffer, size_t& pathLength) noexcept;
}

bool retro::environment(unsigned cmd, void* data) noexcept {
    if (_environment) {
        return _environment(cmd, data);
    } else {
        return false;
    }
}

bool retro::set_pixel_format(retro_pixel_format format) noexcept {
    ZoneScopedN(TracyFunction);
    return environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format);
}

int16_t retro::input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
    ZoneScopedN(TracyFunction);
    if (_input_state) {
        return _input_state(port, device, index, id);
    } else {
        return 0;
    }
}

uint32_t retro::joypad_state(unsigned port) noexcept {
    ZoneScopedN(TracyFunction);
    uint32_t buttons = 0; // Input bits from libretro

    if (_supports_bitmasks) {
        buttons = retro::input_state(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
    } else {
        buttons = 0;
        for (int i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3 + 1); i++)
            buttons |= retro::input_state(port, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
    }

    return buttons;
}

glm::i16vec2 retro::analog_state(unsigned port, unsigned index) noexcept {
    ZoneScopedN(TracyFunction);

    glm::i16vec2 direction;
    direction.x = retro::input_state(port, RETRO_DEVICE_ANALOG, index, RETRO_DEVICE_ID_ANALOG_X);
    direction.y = retro::input_state(port, RETRO_DEVICE_ANALOG, index, RETRO_DEVICE_ID_ANALOG_Y);

    return direction;
}

void retro::input_poll() {
    ZoneScopedN(TracyFunction);
    if (_input_poll) {
        _input_poll();
    }
}

size_t retro::audio_sample_batch(const int16_t* data, size_t frames) {
    ZoneScopedN(TracyFunction);
    if (_audio_sample_batch) {
        return _audio_sample_batch(data, frames);
    } else {
        return 0;
    }
}

void retro::video_refresh(const void* data, unsigned width, unsigned height, size_t pitch) {
    ZoneScopedN(TracyFunction);
    if (_video_refresh) {
        _video_refresh(data, width, height, pitch);
    }
}

bool retro::set_screen_rotation(ScreenOrientation orientation) noexcept {
    ZoneScopedN(TracyFunction);
    bool rotated = false;
    rotated = environment(RETRO_ENVIRONMENT_SET_ROTATION, &orientation);
    return rotated;
}

// reminder: std::string_views are NOT null-terminated!
// Even if they're created from null-terminated strings, the null byte is outside the view
bool retro::set_core_options(const retro_core_options_v2& options) noexcept {
    ZoneScopedN(TracyFunction);
    unsigned version = 0;
    if (!retro::environment(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
        version = 0;

    retro::debug("Frontend reports core options version: {}", version);

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
    if (isShuttingDown)
        return true;

    isShuttingDown = environment(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
    return isShuttingDown;
}

std::optional<retro_microphone_interface> retro::get_microphone_interface() noexcept {
    retro_microphone_interface micInterface {};
    micInterface.interface_version = RETRO_MICROPHONE_INTERFACE_VERSION;

    if (!environment(RETRO_ENVIRONMENT_GET_MICROPHONE_INTERFACE, &micInterface)) {
        return std::nullopt;
    }

    return micInterface;
}

std::optional<bool> retro::is_fastforwarding() noexcept {
    bool fastforwarding = false;
    bool ok = environment(RETRO_ENVIRONMENT_GET_FASTFORWARDING, &fastforwarding);
    return ok ? std::make_optional(fastforwarding) : std::nullopt;
}

std::optional<retro_throttle_state> retro::get_throttle_state() noexcept {
    retro_throttle_state throttleState {};
    bool ok = environment(RETRO_ENVIRONMENT_GET_THROTTLE_STATE, &throttleState);
    return ok ? std::make_optional(throttleState) : std::nullopt;
}

std::optional<std::chrono::microseconds> retro::last_frame_time() noexcept {
    return _lastFrameTime;
}

bool retro::is_variable_updated() noexcept {
    ZoneScopedN(TracyFunction);

    bool updated = false;
    environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);
    return updated;
}

#ifdef TRACY_ENABLE
constexpr uint32_t GetLogColor(retro_log_level level) noexcept {
    switch (level) {
        case RETRO_LOG_DEBUG:
            return tracy::Color::DimGrey;
        case RETRO_LOG_INFO:
            return tracy::Color::White;
        case RETRO_LOG_WARN:
            return tracy::Color::Yellow;
        case RETRO_LOG_ERROR:
            return tracy::Color::Red;
        default:
            return tracy::Color::White;
    }
}
#endif

void retro::fmt_log(retro_log_level level, fmt::string_view fmt, fmt::format_args args) noexcept {
    fmt::basic_memory_buffer<char, 1024> buffer;
    fmt::vformat_to(std::back_inserter(buffer), fmt, args);

    if (buffer[buffer.size() - 1] != '\n')
        // If the string doesn't end with a newline...
        buffer.push_back('\n');

    // vformat_to doesn't append a null terminator, so we have to do it ourselves
    buffer.push_back('\0');

    if (_log) {
        _log(level, buffer.data());
#ifdef TRACY_ENABLE
        if (tracy::ProfilerAvailable()) {
            TracyMessageCS(buffer.data(), buffer.size() - 1, GetLogColor(level), 8);
        }
#endif
    } else {
        fprintf(stderr, "%s\n", buffer.data());
    }
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

        fmt_log(level, "{}", fmt::make_format_args(text));
    } else {
        vfprintf(stderr, fmt, va);
    }
}

bool retro::set_error_message(const char* message, unsigned duration) {
    if (message == nullptr) {
        error("set_error_message: message is null");
        return false;
    }

    if (duration == 0) {
        error("set_error_message: duration is 0");
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

    return set_message(message_ext);
}

bool retro::fmt_message(retro_log_level level, fmt::string_view format, fmt::format_args arg) noexcept {
    string message = fmt::vformat(format, arg);
    struct retro_message_ext message_ext {
        .msg = message.c_str(),
        .duration = retro::DEFAULT_ERROR_DURATION,
        .priority = retro::DEFAULT_ERROR_PRIORITY,
        .level = level,
        .target = RETRO_MESSAGE_TARGET_ALL,
        .type = RETRO_MESSAGE_TYPE_NOTIFICATION,
        .progress = -1
    };

    return set_message(message_ext);
}

bool retro::set_warn_message(const char* message, unsigned duration) {
    if (message == nullptr) {
        error("set_warn_message: message is null");
        return false;
    }

    if (duration == 0) {
        error("set_warn_message: duration is 0");
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

    return set_message(message_ext);
}

bool retro::set_error_message(const char* message) {
    return set_error_message(message, retro::DEFAULT_ERROR_DURATION);
}

bool retro::set_warn_message(const char* message) {
    return set_warn_message(message, retro::DEFAULT_ERROR_DURATION);
}

optional<unsigned> retro::message_interface_version() noexcept {
    unsigned version = UINT_MAX;
    if (environment(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION, &version)) {
        return version;
    }

    return nullopt;
}

bool retro::set_message(const struct retro_message_ext& message) {
    ZoneScopedN(TracyFunction);
    using std::numeric_limits;

    switch (_message_interface_version) {
        // Given that the frontend supports...
        case 0: { // ...the basic messaging interface...
            // Let's match the semantics of RETRO_ENVIRONMENT_SET_MESSAGE, since that's all we have

            if (message.type == RETRO_MESSAGE_TYPE_STATUS || message.type == RETRO_MESSAGE_TYPE_PROGRESS) {
                // retro_message doesn't support on-screen displays,
                // just time-limited messages.
                // So we don't fall back to retro_message in such cases.
                return false;
            }
            float target_refresh_rate = 60.0f; // In FPS
            environment(RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE, &target_refresh_rate);

            struct retro_message msg {
                .msg = message.msg,
                // convert from ms to frames
                .frames = static_cast<unsigned int>((message.duration / 1000) * target_refresh_rate)
            };

            return environment(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
        }
        case UINT_MAX: { // ...no messaging interface...
            if (message.type == RETRO_MESSAGE_TYPE_STATUS || message.type == RETRO_MESSAGE_TYPE_PROGRESS) {
                // retro_message doesn't support on-screen displays,
                // just time-limited messages.
                // So we don't fall back to retro_message in such cases.
                return false;
            }
            if (message.target == RETRO_MESSAGE_TARGET_OSD) {
                return false;
            }

            fmt_log(message.level, "{}", fmt::make_format_args(message.msg));
            return true;
        }
        default:
            // ...a newer interface than we know about...
            // intentional fall-through
        case 1: { // ...the extended messaging interface...
            return environment(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, (void*) &message);
        }
    }
}

bool retro::supports_bitmasks() {
    return _supports_bitmasks;
}

bool retro::get_variable(struct retro_variable* var) {
    return environment(RETRO_ENVIRONMENT_GET_VARIABLE, var);
}

string_view retro::get_variable(std::string_view key) noexcept {
    ZoneScopedN(TracyFunction);
    struct retro_variable var = {key.data(), nullptr};
    if (!environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
        // Get the requested variable. If that failed...
        return {};
    }

    if (!key.empty()) {
        // If we wanted a specific variable...
        return var.value ? var.value : string_view();
    }

    // Return the environment string instead
    return var.key ? var.key : string_view();
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

bool retro::set_geometry(const retro_game_geometry& geometry) noexcept {
    ZoneScopedN(TracyFunction);
    return retro::environment(RETRO_ENVIRONMENT_SET_GEOMETRY, (void*) &geometry);
}

bool retro::set_system_av_info(const retro_system_av_info& av_info) noexcept {
    ZoneScopedN(TracyFunction);
    return retro::environment(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, (void*)&av_info);
}

optional<string_view> retro::username() noexcept {
    ZoneScopedN(TracyFunction);
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
    ZoneScopedN(TracyFunction);
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
    ZoneScopedN(TracyFunction);
    struct retro_device_power power;
    if (!environment(RETRO_ENVIRONMENT_GET_DEVICE_POWER, &power)) {
        return nullopt;
    }

    return power;
}

bool retro::set_hw_render(retro_hw_render_callback& callback) noexcept {
    ZoneScopedN(TracyFunction);

    return environment(RETRO_ENVIRONMENT_SET_HW_RENDER, &callback);
}

optional<string_view> retro::get_save_directory() noexcept {
    return _saveDirLength ? std::make_optional<string_view>(_saveDir, _saveDirLength) : nullopt;
}

optional<string_view> retro::get_save_subdirectory() noexcept {
    return _saveSubdirLength ? std::make_optional<string_view>(_saveSubdir, _saveSubdirLength) : nullopt;
}

optional<string> retro::get_save_subdir_path(std::string_view name) noexcept {
    ZoneScopedN(TracyFunction);

    optional<string_view> savedir = get_save_directory();
    if (!savedir)
        return nullopt;

    if (string_ends_with_size(savedir->data(), SUBDIR_SUFFIX.data(), savedir->size(), SUBDIR_SUFFIX.size())) {
        // If the main save directory already ends in "melonDS DS"...
        return get_save_path(name); // No need to append it
    }

    char path[PATH_MAX] {};
    size_t pathLength = fill_pathname_join_special(path, MELONDSDS_NAME, name.data(), sizeof(path));
    pathname_make_slashes_portable(path);

    return get_save_path(string_view(path, pathLength));
}

optional<string_view> retro::get_system_directory() noexcept {
    return _sysDirLength ? std::make_optional<string_view>(_sysDir, _sysDirLength) : nullopt;
}

optional<string_view> retro::get_system_subdirectory() noexcept {
    return _sysSubdirLength ? std::make_optional<string_view>(_sysSubdir, _sysSubdirLength) : nullopt;
}

optional<string> retro::get_system_subdir_path(std::string_view name) noexcept {
    ZoneScopedN(TracyFunction);
    optional<string_view> sysdir = get_system_directory();
    if (!sysdir)
        return nullopt;

    if (string_ends_with_size(sysdir->data(), SUBDIR_SUFFIX.data(), sysdir->size(), SUBDIR_SUFFIX.size())) {
        // If the main system directory already ends in "melonDS DS"...
        return get_system_path(name); // No need to append it
    }

    char path[PATH_MAX] {};
    size_t pathLength = fill_pathname_join_special(path, MELONDSDS_NAME, name.data(), sizeof(path));
    pathname_make_slashes_portable(path);

    return get_system_path(string_view(path, pathLength));
}

optional<string> retro::get_save_path(string_view name) noexcept {
    ZoneScopedN(TracyFunction);
    optional<string_view> savedir = retro::get_save_directory();
    if (!savedir) {
        // If no save directory is available...
        return nullopt;
    }

    char fullpath[PATH_MAX];
    size_t pathLength = fill_pathname_join_special(fullpath, savedir->data(), name.data(), sizeof(fullpath));

    if (pathLength >= sizeof(fullpath)) {
        // If the path is too long...
        return nullopt;
    }

    pathname_make_slashes_portable(fullpath);
    return string(fullpath);
}

optional<string> retro::get_system_path(string_view name) noexcept {
    ZoneScopedN(TracyFunction);
    optional<string_view> sysdir = retro::get_system_directory();
    if (!sysdir) {
        // If no system directory is available, or the name is empty or not null-terminated...
        return nullopt;
    }

    char fullpath[PATH_MAX];
    size_t pathLength = fill_pathname_join_special(fullpath, sysdir->data(), name.data(), sizeof(fullpath));

    if (pathLength >= sizeof(fullpath)) {
        // If the path is too long...
        return nullopt;
    }

    pathname_make_slashes_portable(fullpath);
    return string(fullpath);
}

bool retro::set_rumble_state(unsigned port, retro_rumble_effect effect, uint16_t strength) noexcept {
    if (!_rumble.set_rumble_state)
        return false;

    return _rumble.set_rumble_state(port, effect, strength);
}

bool retro::set_rumble_state(unsigned port, uint16_t strength) noexcept {
    if (!_rumble.set_rumble_state)
        return false;

    return
        _rumble.set_rumble_state(port, RETRO_RUMBLE_STRONG, strength) &&
        _rumble.set_rumble_state(port, RETRO_RUMBLE_WEAK, strength);
}

bool retro::set_sensor_state(unsigned port, retro_sensor_action action, unsigned rate) noexcept {
    if (!_sensor.set_sensor_state)
        return false;

    return _sensor.set_sensor_state(port, action, rate);
}

std::optional<float> retro::sensor_get_input(unsigned port, unsigned id) noexcept {
    if (!_sensor.get_sensor_input)
        return std::nullopt;

    return _sensor.get_sensor_input(port, id);
}

void retro::env::init() noexcept {
    ZoneScopedN(TracyFunction);
    retro_assert(_environment != nullptr);

    isShuttingDown = false;

    if (_supportsNoGameMode)
        retro::debug("Frontend supports no-game mode.\n");

    if (retro::_supportsPowerStatus)
        retro::debug("Power state available\n");
}

void retro::env::deinit() noexcept {
    ZoneScopedN(TracyFunction);
    _saveDirLength = 0;
    _saveSubdirLength = 0;
    _sysDirLength = 0;
    _sysSubdirLength = 0;
    _environment = nullptr;
    _log = nullptr;
    _supports_bitmasks = false;
    _supportsPowerStatus = false;
    _supportsNoGameMode = false;
    _lastFrameTime = std::nullopt;
    _message_interface_version = UINT_MAX;
}

[[gnu::hot]] static void FrameTimeCallback(retro_usec_t usec) noexcept {
    using namespace retro::env;
    retro::_lastFrameTime = std::chrono::microseconds(usec);
}

// This function might be called multiple times by the frontend,
// and not always with the same value of cb.
PUBLIC_SYMBOL void retro_set_environment(retro_environment_t cb) {
    // Do NOT call Tracy code here, it hasn't been initialized yet.
    //ZoneScopedN("retro_set_environment");
    retro_assert(cb != nullptr);
    using namespace retro;
    retro::_environment = cb;

    // TODO: Handle potential errors with each environment call below
    retro_core_options_update_display_callback update_display_cb {MelonDsDs::UpdateOptionVisibility};
    environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK, &update_display_cb);

    environment(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE, (void*) MelonDsDs::content_overrides);
    environment(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*) MelonDsDs::ports);

    bool yes = true;
    environment(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &yes);

    retro_rumble_interface rumble {nullptr};
    if (environment(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble) && rumble.set_rumble_state) {
        _rumble = rumble;
    }

    if (retro_sensor_interface sensor; environment(RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE, &sensor) && sensor.set_sensor_state) {
        _sensor = sensor;
    }

    retro_log_callback log_callback = {nullptr};
    if (environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log_callback) && log_callback.log) {
        retro::_log = log_callback.log;
        retro::debug("retro_set_environment({})", fmt::ptr(cb));
    } else if (!retro::_log) {
        // retro_set_environment might be called multiple times with different callbacks
        retro::warn("Failed to get log interface");
    }

    retro_frame_time_callback frame_time {FrameTimeCallback, static_cast<retro_usec_t>(1000000.0 / MelonDsDs::FPS)};
    environment(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &frame_time);

    retro_get_proc_address_interface get_proc_address {MelonDsDs::GetRetroProcAddress};
    environment(RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK, &get_proc_address);

    retro::_supports_bitmasks |= environment(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, nullptr);
    retro::_supportsPowerStatus |= environment(RETRO_ENVIRONMENT_GET_DEVICE_POWER, nullptr);

    if (retro::_message_interface_version == UINT_MAX && !environment(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION, &retro::_message_interface_version)) {
        retro::_message_interface_version = UINT_MAX;
    }

    retro::debug("Frontend report a message API version {}", retro::_message_interface_version);

    if (const char* save_dir = nullptr; environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir) {
        // First copy the returned path into the buffer we'll use...
        strlcpy(_saveDir, save_dir, sizeof(_saveDir));
        NormalizePath(_saveDir, _saveDirLength);

        if (string_ends_with_size(_saveDir, SUBDIR_SUFFIX.data(), _saveDirLength, SUBDIR_SUFFIX.size())) {
            // If the frontend-provided save directory already ends with "/melonDS DS"...

            memcpy(_saveSubdir, _saveDir, _saveDirLength);
            _saveSubdirLength = _saveDirLength;
        }
        else {
            _saveSubdirLength = fill_pathname_join_special(_saveSubdir, save_dir, MELONDSDS_NAME, sizeof(_saveDir));
            pathname_make_slashes_portable(_saveSubdir);
        }

        retro::info("Save directory: \"{}\"", _saveDir);
        if (path_mkdir(_saveSubdir)) {
            retro::info("melonDS DS save subdirectory: \"{}\"", _saveSubdir);
        }
        else {
            retro::error("Failed to create melonDS DS save subdirectory at \"{}\"", _saveSubdir);
        }
    }

    if (const char* system_dir = nullptr; environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir) {
        // First copy the returned path into the buffer we'll use...
        strlcpy(_sysDir, system_dir, sizeof(_sysDir));
        NormalizePath(_sysDir, _sysDirLength);

        if (string_ends_with_size(_sysDir, SUBDIR_SUFFIX.data(), _sysDirLength, SUBDIR_SUFFIX.size())) {
            // If the frontend-provided system directory already ends with "/melonDS DS"...

            memcpy(_sysSubdir, _sysDir, _sysDirLength);
            _sysSubdirLength = _sysDirLength;
        }
        else {
            _sysSubdirLength = fill_pathname_join_special(_sysSubdir, system_dir, MELONDSDS_NAME, sizeof(_sysDir));
            pathname_make_slashes_portable(_sysSubdir);
        }

        retro::info("System directory: \"{}\"", _sysDir);
        if (path_mkdir(_sysSubdir)) {
            retro::info("melonDS DS system subdirectory: \"{}\"", _sysSubdir);
        }
        else {
            retro::error("Failed to create melonDS DS system subdirectory at \"{}\"", _sysSubdir);
        }
    }

    environment(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void*) MelonDsDs::subsystems);

    retro_vfs_interface_info vfs { .required_interface_version = PATH_REQUIRED_VFS_VERSION, .iface = nullptr };
    if (environment(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs)) {
        debug("Requested VFS interface version {}, got {}", PATH_REQUIRED_VFS_VERSION, vfs.required_interface_version);
        path_vfs_init(&vfs);
        filestream_vfs_init(&vfs);
        dirent_vfs_init(&vfs);
    } else {
        warn("Could not get VFS interface {}, falling back to libretro-common defaults", PATH_REQUIRED_VFS_VERSION);
    }

    yes = true;
    retro::_supportsNoGameMode |= environment(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &yes);
}

void retro::NormalizePath(std::span<char> buffer, size_t& pathLength) noexcept {
    // Ensure all slashes are forward...
    pathname_make_slashes_portable(buffer.data());

    // Then if the path ends in a slash, strip it.
    pathLength = strnlen(buffer.data(), buffer.size());
    if (pathLength > 0 && buffer[pathLength - 1] == '/') {
        // If this path is non-empty and ends with a slash...
        buffer[pathLength - 1] = '\0';
        pathLength--;
    }
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