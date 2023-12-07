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

#ifndef MELONDSDS_PARSE_HPP
#define MELONDSDS_PARSE_HPP

#include <optional>
#include <string_view>

#include "constants.hpp"
#include "config.hpp"
#include "config/types.hpp"

#include "tracy.hpp"

namespace MelonDsDs {
    using namespace melonds;

    constexpr std::optional<bool> ParseBoolean(std::string_view value) noexcept {
        if (value == config::values::ENABLED) return true;
        if (value == config::values::DISABLED) return false;
        return std::nullopt;
    }

    template<typename T>
    std::optional<T> ParseIntegerInRange(std::string_view value, T min, T max) noexcept {
        ZoneScopedN(TracyFunction);
        if (min > max) return std::nullopt;
        if (value.empty()) return std::nullopt;

        T parsed_number = 0;
        std::from_chars_result result = std::from_chars(value, &value.back(), parsed_number);

        if (result.ec != std::errc()) return std::nullopt;
        if (parsed_number < min || parsed_number > max) return std::nullopt;

        return parsed_number;
    }

    template<typename T>
    std::optional<T> ParseIntegerInList(std::string_view value, const std::initializer_list<T>& list) noexcept {
        ZoneScopedN(TracyFunction);
        if (value.empty()) return std::nullopt;

        T parsed_number = 0;
        std::from_chars_result result = std::from_chars(value, &value.back(), parsed_number);

        if (result.ec != std::errc()) return std::nullopt;
        for (T t: list) {
            if (parsed_number == t) return parsed_number;
        }

        return std::nullopt;
    }

    constexpr std::optional<melonds::BootMode> ParseBootMode(std::string_view value) noexcept {
        if (value == config::values::NATIVE) return BootMode::Native;
        if (value == config::values::DIRECT) return BootMode::Direct;
        return std::nullopt;
    }

    constexpr std::optional<melonds::SysfileMode> ParseSysfileMode(std::string_view value) noexcept {
        if (value == config::values::NATIVE) return SysfileMode::Native;
        if (value == config::values::BUILT_IN) return SysfileMode::BuiltIn;
        return std::nullopt;
    }

    constexpr std::optional<melonds::AlarmMode> ParseAlarmMode(std::string_view value) noexcept {
        if (value == config::values::DISABLED) return AlarmMode::Disabled;
        if (value == config::values::ENABLED) return AlarmMode::Enabled;
        if (value == config::values::DEFAULT) return AlarmMode::Default;
        return std::nullopt;
    }

    constexpr std::optional<melonds::UsernameMode> ParseUsernameMode(std::string_view value) noexcept {
        if (value.empty() || value == config::values::firmware::DEFAULT_USERNAME) return UsernameMode::MelonDSDS;
        if (value == config::values::firmware::FIRMWARE_USERNAME) return UsernameMode::Firmware;
        if (value == config::values::firmware::GUESS_USERNAME) return UsernameMode::Guess;
        return std::nullopt;
    }

    constexpr std::optional<melonds::Renderer> ParseRenderer(std::string_view value) noexcept {
        if (value == config::values::SOFTWARE) return melonds::Renderer::Software;
        if (value == config::values::OPENGL) return melonds::Renderer::OpenGl;
        return std::nullopt;
    }

    constexpr std::optional<melonds::CursorMode> ParseCursorMode(std::string_view value) noexcept {
        if (value == config::values::DISABLED) return melonds::CursorMode::Never;
        if (value == config::values::TOUCHING) return melonds::CursorMode::Touching;
        if (value == config::values::TIMEOUT) return melonds::CursorMode::Timeout;
        if (value == config::values::ALWAYS) return melonds::CursorMode::Always;
        return std::nullopt;
    }

    constexpr std::optional<melonds::ConsoleType> ParseConsoleType(std::string_view value) noexcept {
        if (value == config::values::DS) return melonds::ConsoleType::DS;
        if (value == config::values::DSI) return melonds::ConsoleType::DSi;
        return std::nullopt;
    }

    constexpr std::optional<melonds::NetworkMode> ParseNetworkMode(std::string_view value) noexcept {
        if (value == config::values::DISABLED) return melonds::NetworkMode::None;
        if (value == config::values::DIRECT) return melonds::NetworkMode::Direct;
        if (value == config::values::INDIRECT) return melonds::NetworkMode::Indirect;
        return std::nullopt;
    }

    constexpr std::optional<melonds::ScreenLayout> ParseScreenLayout(std::string_view value) noexcept {
        using melonds::ScreenLayout;
        if (value == config::values::TOP_BOTTOM) return ScreenLayout::TopBottom;
        if (value == config::values::BOTTOM_TOP) return ScreenLayout::BottomTop;
        if (value == config::values::LEFT_RIGHT) return ScreenLayout::LeftRight;
        if (value == config::values::RIGHT_LEFT) return ScreenLayout::RightLeft;
        if (value == config::values::TOP) return ScreenLayout::TopOnly;
        if (value == config::values::BOTTOM) return ScreenLayout::BottomOnly;
        if (value == config::values::HYBRID_TOP) return ScreenLayout::HybridTop;
        if (value == config::values::HYBRID_BOTTOM) return ScreenLayout::HybridBottom;
        if (value == config::values::ROTATE_LEFT) return ScreenLayout::TurnLeft;
        if (value == config::values::ROTATE_RIGHT) return ScreenLayout::TurnRight;
        if (value == config::values::UPSIDE_DOWN) return ScreenLayout::UpsideDown;

        return std::nullopt;
    }

    constexpr std::optional<HybridSideScreenDisplay> ParseHybridSideScreenDisplay(std::string_view value) noexcept {
        using melonds::ScreenLayout;
        if (value == config::values::ONE) return melonds::HybridSideScreenDisplay::One;
        if (value == config::values::BOTH) return melonds::HybridSideScreenDisplay::Both;

        return std::nullopt;
    }

    constexpr std::optional<FirmwareLanguage> ParseLanguage(std::string_view value) noexcept {
        if (value == config::values::AUTO) return melonds::FirmwareLanguage::Auto;
        if (value == config::values::DEFAULT) return melonds::FirmwareLanguage::Default;
        if (value == config::values::JAPANESE) return melonds::FirmwareLanguage::Japanese;
        if (value == config::values::ENGLISH) return melonds::FirmwareLanguage::English;
        if (value == config::values::FRENCH) return melonds::FirmwareLanguage::French;
        if (value == config::values::GERMAN) return melonds::FirmwareLanguage::German;
        if (value == config::values::ITALIAN) return melonds::FirmwareLanguage::Italian;
        if (value == config::values::SPANISH) return melonds::FirmwareLanguage::Spanish;

        return std::nullopt;
    }

    constexpr std::optional<MicInputMode> ParseMicInputMode(std::string_view value) noexcept {
        if (value == config::values::MICROPHONE) return MicInputMode::HostMic;
        if (value == config::values::NOISE) return MicInputMode::WhiteNoise;
        if (value == config::values::SILENCE) return MicInputMode::None;

        return std::nullopt;
    }

    constexpr std::optional<melonds::TouchMode> ParseTouchMode(std::string_view value) noexcept {
        if (value == config::values::AUTO) return TouchMode::Auto;
        if (value == config::values::TOUCH) return TouchMode::Pointer;
        if (value == config::values::JOYSTICK) return TouchMode::Joystick;

        return std::nullopt;
    }

    std::optional<melonDS::IpAddress> ParseIpAddress(std::string_view value) noexcept;
}
#endif // MELONDSDS_PARSE_HPP
