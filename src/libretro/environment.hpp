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
#include <fmt/format.h>

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

    bool set_pixel_format(retro_pixel_format format) noexcept;
    bool set_screen_rotation(ScreenOrientation orientation) noexcept;
    bool set_core_options(const retro_core_options_v2& options) noexcept;

    [[nodiscard]] bool is_variable_updated() noexcept;

    void fmt_log(retro_log_level level, fmt::string_view fmt, fmt::format_args args) noexcept;

    template <typename... T>
    void debug(fmt::format_string<T...> format, T&&... args) noexcept {
        fmt_log(RETRO_LOG_DEBUG, format, fmt::make_format_args(args...));
    }

    template <typename... T>
    void info(fmt::format_string<T...> format, T&&... args) noexcept {
        fmt_log(RETRO_LOG_INFO, format, fmt::make_format_args(args...));
    }

    template <typename... T>
    void warn(fmt::format_string<T...> format, T&&... args) noexcept {
        fmt_log(RETRO_LOG_WARN, format, fmt::make_format_args(args...));
    }

    template <typename... T>
    void error(fmt::format_string<T...> format, T&&... args) noexcept {
        fmt_log(RETRO_LOG_ERROR, format, fmt::make_format_args(args...));
    }

    [[gnu::format(printf, 2, 0)]]
    [[gnu::access(read_only, 2)]]
    void vlog(enum retro_log_level level, const char* fmt, va_list va) noexcept;
    std::optional<unsigned> message_interface_version() noexcept;
    bool set_message(const struct retro_message_ext &message);
    [[gnu::access(read_only, 1)]]
    bool set_error_message(const char* message, unsigned duration);
    bool set_error_message(const char* message);
    bool fmt_message(retro_log_level level, fmt::string_view format, fmt::format_args args) noexcept;

    template <typename... T>
    bool set_error_message(fmt::format_string<T...> format, T&&... args) noexcept {
        return fmt_message(RETRO_LOG_ERROR, format, fmt::make_format_args(args...));
    }

    template <typename... T>
    bool set_warn_message(fmt::format_string<T...> format, T&&... args) noexcept {
        return fmt_message(RETRO_LOG_WARN, format, fmt::make_format_args(args...));
    }

    bool set_warn_message(const char* message);
    bool set_warn_message(const char* message, unsigned duration);
    bool get_variable(struct retro_variable *variable);
    const char* get_variable(const char *key);
    bool set_variable(const char* key, const char* value);
    std::optional<retro_language> get_language() noexcept;
    std::optional<std::string> username() noexcept;
    void set_option_visible(const char* key, bool visible) noexcept;
    bool supports_power_status() noexcept;
    std::optional<retro_device_power> get_device_power() noexcept;

    bool supports_bitmasks();
    void input_poll();
    int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id);
    size_t audio_sample_batch(const int16_t *data, size_t frames);
    void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch);

    bool shutdown() noexcept;

    const std::optional<std::string>& get_save_directory();
    const std::optional<std::string>& get_system_directory();
    std::optional<std::string> get_system_path(const std::string_view& name) noexcept;
    const std::optional<std::string>& get_system_subdirectory();
    std::optional<std::string> get_system_subdir_path(const std::string_view& name) noexcept;

    namespace env {
        void init() noexcept;
        void deinit() noexcept;
    }
}

#endif //MELONDS_DS_ENVIRONMENT_HPP
