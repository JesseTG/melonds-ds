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

#include <cstdarg>
#include <cstdio>

#include <libretro.h>
#include <streams/file_stream.h>
#include <cstring>


#include "environment.hpp"
#include "microphone.hpp"
#include "info.hpp"
#include "libretro.hpp"
#include "config.hpp"

using std::string;
using std::optional;
using std::nullopt;

namespace retro {
    static void set_core_options();
    static retro_environment_t _environment;
    static retro_video_refresh_t _video_refresh;
    static retro_audio_sample_batch_t _audio_sample_batch;
    static retro_input_poll_t _input_poll;
    static retro_input_state_t _input_state;
    static retro_log_printf_t _log;
    static bool _supports_bitmasks;
    static bool _config_categories_supported;
    static bool _supports_power_status;

    // Cached so that the save directory won't change during a session
    static optional<string> _save_directory;
    static optional<string> _system_directory;

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
    if (_input_state) {
        return _input_state(port, device, index, id);
    } else {
        return 0;
    }
}

void retro::input_poll() {
    if (_input_poll) {
        _input_poll();
    }
}

size_t retro::audio_sample_batch(const int16_t* data, size_t frames) {
    if (_audio_sample_batch) {
        return _audio_sample_batch(data, frames);
    } else {
        return 0;
    }
}

void retro::video_refresh(const void* data, unsigned width, unsigned height, size_t pitch) {
    if (_video_refresh) {
        _video_refresh(data, width, height, pitch);
    }
}

bool retro::shutdown() noexcept {
    return environment(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
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

    unsigned message_interface_version = numeric_limits<unsigned>::max();
    environment(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION, &message_interface_version);

    switch (message_interface_version) {
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
        case numeric_limits<unsigned>::max(): { // ...no messaging interface at all...
            log(message->level, "%s", message->msg);
            return false;
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

    return language;
}

const optional<string>& retro::get_save_directory() {
    return _save_directory;
}

const optional<string>& retro::get_system_directory() {
    return _system_directory;
}

int retro::TimeToPowerStatusUpdate;

bool retro::supports_power_status() noexcept {
    return _supports_power_status;
}

optional<struct retro_device_power_status> retro::get_power_status() noexcept {
    struct retro_device_power_status power_status;
    bool success = environment(RETRO_ENVIRONMENT_GET_POWER_STATUS, &power_status);
    return success ? std::make_optional(power_status) : nullopt;
}

void retro::clear_environment() {
    _save_directory = nullopt;
    _system_directory = nullopt;
    microphone::clear_interface();
}

PUBLIC_SYMBOL void retro_set_environment(retro_environment_t cb) {
    using retro::environment;
    retro::_environment = cb;

    retro::set_core_options();

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

    retro::_supports_bitmasks = environment(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, nullptr);

    const char* save_dir = nullptr;
    if (retro::environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir) {
        retro::log(RETRO_LOG_INFO, "Save directory: \"%s\"", save_dir);
        retro::_save_directory = save_dir;
    }

    const char* system_dir = nullptr;
    if (retro::environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir) {
        retro::log(RETRO_LOG_INFO, "System directory: \"%s\"", system_dir);
        retro::_system_directory = system_dir;
    }

    environment(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void*) melonds::subsystems);

    struct retro_vfs_interface_info vfs{};
    vfs.required_interface_version = FILESTREAM_REQUIRED_VFS_VERSION;
    vfs.iface = nullptr;
    if (environment(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs))
        filestream_vfs_init(&vfs);

    bool supports_no_game = true;
    if (environment(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &supports_no_game))
        retro::log(RETRO_LOG_INFO, "Frontend supports no-game mode.");

    retro::microphone::init_interface();

    retro::_supports_power_status |= environment(RETRO_ENVIRONMENT_GET_POWER_STATUS, nullptr);
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

static void retro::set_core_options() {
    unsigned version = 0;

    if (!_environment)
        return;

    _config_categories_supported = false;

    if (!environment(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
        version = 0;

    if (version >= 2) {
#ifndef HAVE_NO_LANGEXTRA
        unsigned language = 0;
        struct retro_core_options_v2_intl core_options_intl{
            &melonds::options_us,
            nullptr
        };

        if (environment(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
            (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
            core_options_intl.local = melonds::options_intl[language];

        _config_categories_supported = environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL, &core_options_intl);
#else
        _config_categories_supported = environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options_us);
#endif
    } else {
        size_t option_index = 0;
        size_t num_options = 0;
        struct retro_core_option_definition* option_v1_defs_us = nullptr;
#ifndef HAVE_NO_LANGEXTRA
        size_t num_options_intl = 0;
        unsigned language = 0;
        struct retro_core_option_v2_definition* option_defs_intl = nullptr;
        struct retro_core_option_definition* option_v1_defs_intl = nullptr;
        struct retro_core_options_intl core_options_v1_intl;
#endif
        struct retro_variable* variables = nullptr;
        char** values_buf = nullptr;

        /* Determine total number of options */
        while (true) {
            if (melonds::option_defs_us[num_options].key)
                num_options++;
            else
                break;
        }

        if (version >= 1) {
            /* Allocate US array */
            option_v1_defs_us = (struct retro_core_option_definition*)
                calloc(num_options + 1, sizeof(struct retro_core_option_definition));

            /* Copy parameters from option_defs_us array */
            for (int i = 0; i < num_options; i++) {
                struct retro_core_option_v2_definition* option_def_us = &melonds::option_defs_us[i];
                struct retro_core_option_value* option_values = option_def_us->values;
                struct retro_core_option_definition* option_v1_def_us = &option_v1_defs_us[i];
                struct retro_core_option_value* option_v1_values = option_v1_def_us->values;

                option_v1_def_us->key = option_def_us->key;
                option_v1_def_us->desc = option_def_us->desc;
                option_v1_def_us->info = option_def_us->info;
                option_v1_def_us->default_value = option_def_us->default_value;

                /* Values must be copied individually... */
                while (option_values->value) {
                    option_v1_values->value = option_values->value;
                    option_v1_values->label = option_values->label;

                    option_values++;
                    option_v1_values++;
                }
            }

#ifndef HAVE_NO_LANGEXTRA
            if (environment(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
                (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH) &&
                melonds::options_intl[language])
                option_defs_intl = melonds::options_intl[language]->definitions;

            if (option_defs_intl) {
                /* Determine number of intl options */
                while (true) {
                    if (option_defs_intl[num_options_intl].key)
                        num_options_intl++;
                    else
                        break;
                }

                /* Allocate intl array */
                option_v1_defs_intl = (struct retro_core_option_definition*)
                    calloc(num_options_intl + 1, sizeof(struct retro_core_option_definition));

                /* Copy parameters from option_defs_intl array */
                for (int i = 0; i < num_options_intl; i++) {
                    struct retro_core_option_v2_definition* option_def_intl = &option_defs_intl[i];
                    struct retro_core_option_value* option_values = option_def_intl->values;
                    struct retro_core_option_definition* option_v1_def_intl = &option_v1_defs_intl[i];
                    struct retro_core_option_value* option_v1_values = option_v1_def_intl->values;

                    option_v1_def_intl->key = option_def_intl->key;
                    option_v1_def_intl->desc = option_def_intl->desc;
                    option_v1_def_intl->info = option_def_intl->info;
                    option_v1_def_intl->default_value = option_def_intl->default_value;

                    /* Values must be copied individually... */
                    while (option_values->value) {
                        option_v1_values->value = option_values->value;
                        option_v1_values->label = option_values->label;

                        option_values++;
                        option_v1_values++;
                    }
                }
            }

            core_options_v1_intl.us = option_v1_defs_us;
            core_options_v1_intl.local = option_v1_defs_intl;

            environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_v1_intl);
#else
            environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
#endif
        } else {
            /* Allocate arrays */
            variables = (struct retro_variable*) calloc(num_options + 1,
                                                        sizeof(struct retro_variable));
            values_buf = (char**) calloc(num_options, sizeof(char*));

            if (!variables || !values_buf)
                goto error;

            /* Copy parameters from option_defs_us array */
            for (int i = 0; i < num_options; i++) {
                const char* key = melonds::option_defs_us[i].key;
                const char* desc = melonds::option_defs_us[i].desc;
                const char* default_value = melonds::option_defs_us[i].default_value;
                struct retro_core_option_value* values = melonds::option_defs_us[i].values;
                size_t buf_len = 3;
                size_t default_index = 0;

                values_buf[i] = nullptr;

                if (desc) {
                    size_t num_values = 0;

                    /* Determine number of values */
                    while (true) {
                        if (values[num_values].value) {
                            /* Check if this is the default value */
                            if (default_value)
                                if (strcmp(values[num_values].value, default_value) == 0)
                                    default_index = num_values;

                            buf_len += strlen(values[num_values].value);
                            num_values++;
                        } else
                            break;
                    }

                    /* Build values string */
                    if (num_values > 0) {
                        buf_len += num_values - 1;
                        buf_len += strlen(desc);

                        values_buf[i] = (char*) calloc(buf_len, sizeof(char));
                        if (!values_buf[i])
                            goto error;

                        strcpy(values_buf[i], desc);
                        strcat(values_buf[i], "; ");

                        /* Default value goes first */
                        strcat(values_buf[i], values[default_index].value);

                        /* Add remaining values */
                        for (int j = 0; j < num_values; j++) {
                            if (j != default_index) {
                                strcat(values_buf[i], "|");
                                strcat(values_buf[i], values[j].value);
                            }
                        }
                    }
                }

                variables[option_index].key = key;
                variables[option_index].value = values_buf[i];
                option_index++;
            }

            /* Set variables */
            environment(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
        }

        error:
        /* Clean up */

        if (option_v1_defs_us) {
            free(option_v1_defs_us);
            option_v1_defs_us = NULL;
        }

#ifndef HAVE_NO_LANGEXTRA
        if (option_v1_defs_intl) {
            free(option_v1_defs_intl);
            option_v1_defs_intl = NULL;
        }
#endif

        if (values_buf) {
            for (int i = 0; i < num_options; i++) {
                if (values_buf[i]) {
                    free(values_buf[i]);
                    values_buf[i] = NULL;
                }
            }

            free(values_buf);
            values_buf = NULL;
        }

        if (variables) {
            free(variables);
            variables = NULL;
        }
    }
}