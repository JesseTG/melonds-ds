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

#ifndef MELONDS_DS_ENVIRONMENT_HPP
#define MELONDS_DS_ENVIRONMENT_HPP

#include <optional>
#include <string>
#include <libretro.h>

namespace retro {
    constexpr unsigned DEFAULT_ERROR_DURATION = 5000; // in ms
    constexpr unsigned DEFAULT_ERROR_PRIORITY = 3;

    /// For use by other parts of the core
    bool environment(unsigned cmd, void *data);

    void log(enum retro_log_level level, const char *fmt, ...);
    void log(enum retro_log_level level, const char* fmt, va_list va);
    bool set_message(const struct retro_message_ext *message);
    bool set_error_message(const char* message, unsigned duration);
    bool set_error_message(const char* message);
    bool set_warn_message(const char* message);
    bool set_warn_message(const char* message, unsigned duration);
    bool get_variable(struct retro_variable *variable);
    const char* get_variable(const char *key);
    bool set_variable(const char* key, const char* value);

    bool supports_bitmasks();
    void input_poll();
    int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id);
    size_t audio_sample_batch(const int16_t *data, size_t frames);
    void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch);

    const std::optional<std::string>& get_save_directory();
    const std::optional<std::string>& get_system_directory();

    const std::optional<struct retro_game_info>& get_loaded_nds_info();
    const std::optional<struct retro_game_info_ext>& get_loaded_nds_info_ext();
    const std::optional<struct retro_game_info>& get_loaded_gba_info();
    const std::optional<struct retro_game_info_ext>& get_loaded_gba_info_ext();

    void set_loaded_content_info(const struct retro_game_info *nds_info, const struct retro_game_info *gba_info);

    void clear_environment();
}

#endif //MELONDS_DS_ENVIRONMENT_HPP
