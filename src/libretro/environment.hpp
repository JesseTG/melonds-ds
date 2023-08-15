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

// Added temporarily
#ifndef RETRO_ENVIRONMENT_GET_DEVICE_POWER
extern "C" {
/**
 * Describes how a device is being powered.
 * @see RETRO_ENVIRONMENT_GET_DEVICE_POWER
 */
enum retro_power_state
{
    /**
     * Indicates that the frontend cannot report its power state at this time,
     * most likely due to a lack of support.
     *
     * \c RETRO_ENVIRONMENT_GET_DEVICE_POWER will not return this value;
     * instead, the environment callback will return \c false.
     */
    RETRO_POWERSTATE_UNKNOWN = 0,

    /**
     * Indicates that the device is running on its battery.
     * Usually applies to portable devices such as handhelds, laptops, and smartphones.
     */
    RETRO_POWERSTATE_DISCHARGING,

    /**
     * Indicates that the device's battery is currently charging.
     */
    RETRO_POWERSTATE_CHARGING,

    /**
     * Indicates that the device is connected to a power source
     * and that its battery has finished charging.
     */
    RETRO_POWERSTATE_CHARGED,

    /**
     * Indicates that the device is connected to a power source
     * and that it does not have a battery.
     * This usually suggests a desktop computer or a non-portable game console.
     */
    RETRO_POWERSTATE_PLUGGED_IN
};

/**
 * Indicates that an estimate is not available for the battery level or time remaining,
 * even if the actual power state is known.
 */
#define RETRO_POWERSTATE_NO_ESTIMATE (-1)

/**
 * Describes the power state of the device running the frontend.
 * @see RETRO_ENVIRONMENT_GET_DEVICE_POWER
 */
struct retro_device_power
{
    /**
     * The current state of the frontend's power usage.
     */
    enum retro_power_state state;

    /**
     * A rough estimate of the amount of time remaining (in seconds)
     * before the device powers off.
     * This value depends on a variety of factors,
     * so it is not guaranteed to be accurate.
     *
     * Will be set to \c RETRO_POWERSTATE_NO_ESTIMATE if \c state does not equal \c RETRO_POWERSTATE_DISCHARGING.
     * May still be set to \c RETRO_POWERSTATE_NO_ESTIMATE if the frontend is unable to provide an estimate.
     */
    int seconds;

    /**
     * The approximate percentage of battery charge,
     * ranging from 0 to 100 (inclusive).
     * The device may power off before this reaches 0.
     *
     * The user might have configured their device
     * to stop charging before the battery is full,
     * so do not assume that this will be 100 in the \c RETRO_POWERSTATE_CHARGED state.
     */
    int8_t percent;
};

#define RETRO_ENVIRONMENT_GET_DEVICE_POWER (77 | RETRO_ENVIRONMENT_EXPERIMENTAL)
}
#endif


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
    bool set_core_options(const retro_core_options_v2& options) noexcept;

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
    const std::optional<std::string>& get_system_subdirectory();

    void clear_environment();
}

#endif //MELONDS_DS_ENVIRONMENT_HPP
