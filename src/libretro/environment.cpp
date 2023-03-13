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
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <cstdarg>
#include <cstdio>

#include <libretro.h>
#include <streams/file_stream.h>
#include <cstring>


#include "environment.hpp"
#include "info.hpp"
#include "libretro.hpp"
#include "config.hpp"

namespace retro {
    static void set_core_options();

    static retro_environment_t _environment;
    static retro_video_refresh_t _video_refresh;
    static retro_audio_sample_batch_t _audio_sample_batch;
    static retro_input_poll_t _input_poll;
    static retro_input_state_t _input_state;
    struct retro_log_callback _log;
    static bool _supports_bitmasks;
    static bool _config_categories_supported;

    bool environment(unsigned cmd, void *data) {
        if (_environment) {
            return _environment(cmd, data);
        } else {
            return false;
        }
    }

    int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
        if (_input_state) {
            return _input_state(port, device, index, id);
        }
        else {
            return 0;
        }
    }

    void input_poll() {
        if (_input_poll) {
            _input_poll();
        }
    }

    size_t audio_sample_batch(const int16_t *data, size_t frames)
    {
        if (_audio_sample_batch)
        {
            _audio_sample_batch(data, frames);
        }
        else
        {
            return 0;
        }
    }

    void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch)
    {
        if (_video_refresh)
        {
            _video_refresh(data, width, height, pitch);
        }
    }

    void log(enum retro_log_level level, const char *fmt, ...) {
        va_list va;
        va_start(va, fmt);
        if (_log.log) {
            _log.log(level, fmt, va);
        } else {
            vfprintf(stderr, fmt, va);
        }
        va_end(va);
    }

    bool supports_bitmasks() {
        return _supports_bitmasks;
    }
}

PUBLIC_SYMBOL void retro_set_environment(retro_environment_t cb) {
    using retro::environment;
    retro::_environment = cb;

    retro::set_core_options();

    // TODO: Handle potential errors with each environment call below
    struct retro_core_options_update_display_callback update_display_cb{melonds::update_option_visibility};
    environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK, &update_display_cb);

    environment(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE, (void *) melonds::content_overrides);
    environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &retro::_log);
    environment(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void *) melonds::ports);

    retro::_supports_bitmasks = environment(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, nullptr);


    environment(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void *) melonds::subsystems);

    struct retro_vfs_interface_info vfs{};
    vfs.required_interface_version = FILESTREAM_REQUIRED_VFS_VERSION;
    vfs.iface = nullptr;
    if (environment(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs))
        filestream_vfs_init(&vfs);
}

PUBLIC_SYMBOL void retro_set_video_refresh(retro_video_refresh_t video_refresh) {
    retro::_video_refresh = video_refresh;
}

PUBLIC_SYMBOL void retro_set_audio_sample(retro_audio_sample_t audio_sample) {
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
        struct retro_core_option_definition *option_v1_defs_us = nullptr;
#ifndef HAVE_NO_LANGEXTRA
        size_t num_options_intl = 0;
        unsigned language = 0;
        struct retro_core_option_v2_definition *option_defs_intl = nullptr;
        struct retro_core_option_definition *option_v1_defs_intl = nullptr;
        struct retro_core_options_intl core_options_v1_intl;
#endif
        struct retro_variable *variables = nullptr;
        char **values_buf = nullptr;

        /* Determine total number of options */
        while (true) {
            if (melonds::option_defs_us[num_options].key)
                num_options++;
            else
                break;
        }

        if (version >= 1) {
            /* Allocate US array */
            option_v1_defs_us = (struct retro_core_option_definition *)
                    calloc(num_options + 1, sizeof(struct retro_core_option_definition));

            /* Copy parameters from option_defs_us array */
            for (int i = 0; i < num_options; i++) {
                struct retro_core_option_v2_definition *option_def_us = &melonds::option_defs_us[i];
                struct retro_core_option_value *option_values = option_def_us->values;
                struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
                struct retro_core_option_value *option_v1_values = option_v1_def_us->values;

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
                option_v1_defs_intl = (struct retro_core_option_definition *)
                        calloc(num_options_intl + 1, sizeof(struct retro_core_option_definition));

                /* Copy parameters from option_defs_intl array */
                for (int i = 0; i < num_options_intl; i++) {
                    struct retro_core_option_v2_definition *option_def_intl = &option_defs_intl[i];
                    struct retro_core_option_value *option_values = option_def_intl->values;
                    struct retro_core_option_definition *option_v1_def_intl = &option_v1_defs_intl[i];
                    struct retro_core_option_value *option_v1_values = option_v1_def_intl->values;

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
            variables = (struct retro_variable *) calloc(num_options + 1,
                                                         sizeof(struct retro_variable));
            values_buf = (char **) calloc(num_options, sizeof(char *));

            if (!variables || !values_buf)
                goto error;

            /* Copy parameters from option_defs_us array */
            for (int i = 0; i < num_options; i++) {
                const char *key = melonds::option_defs_us[i].key;
                const char *desc = melonds::option_defs_us[i].desc;
                const char *default_value = melonds::option_defs_us[i].default_value;
                struct retro_core_option_value *values = melonds::option_defs_us[i].values;
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

                        values_buf[i] = (char *) calloc(buf_len, sizeof(char));
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