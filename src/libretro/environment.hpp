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

    enum class ScreenOrientation {
        Normal = 0,
        RotatedLeft = 1,
        UpsideDown = 2,
        RotatedRight = 3,
    };

    /// For use by other parts of the core
    bool environment(unsigned cmd, void *data) noexcept;

    bool set_screen_rotation(ScreenOrientation orientation) noexcept;

    [[nodiscard]] bool is_variable_updated() noexcept;

    void log(enum retro_log_level level, const char *fmt, ...) noexcept;
    void debug(const char *fmt, ...) noexcept;
    void info(const char *fmt, ...) noexcept;
    void warn(const char *fmt, ...) noexcept;
    void error(const char *fmt, ...) noexcept;
    void vlog(enum retro_log_level level, const char* fmt, va_list va) noexcept;
    bool set_message(const struct retro_message_ext *message);
    bool set_error_message(const char* message, unsigned duration);
    bool set_error_message(const char* message);
    bool set_warn_message(const char* message);
    bool set_warn_message(const char* message, unsigned duration);
    bool get_variable(struct retro_variable *variable);
    const char* get_variable(const char *key);
    bool set_variable(const char* key, const char* value);
    std::optional<retro_language> get_language() noexcept;
    void set_option_visible(const char* key, bool visible) noexcept;

    bool supports_bitmasks();
    void input_poll();
    int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id);
    size_t audio_sample_batch(const int16_t *data, size_t frames);
    void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch);

    bool shutdown() noexcept;

    const std::optional<std::string>& get_save_directory();
    const std::optional<std::string>& get_system_directory();
    const std::optional<std::string>& get_system_subdirectory();

    void clear_environment();
}

#endif //MELONDS_DS_ENVIRONMENT_HPP
