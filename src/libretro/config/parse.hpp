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
#include <SPU.h>
#include <string_view>

#include "constants.hpp"
#include "config/types.hpp"

#include "tracy.hpp"

namespace MelonDsDs {
    using namespace MelonDsDs;

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
        std::from_chars_result result = std::from_chars(value.data(), value.data() + value.size(), parsed_number);

        if (result.ec != std::errc()) return std::nullopt;
        if (parsed_number < min || parsed_number > max) return std::nullopt;

        return parsed_number;
    }

    template<typename T>
    std::optional<T> ParseIntegerInList(std::string_view value, const std::initializer_list<T>& list) noexcept {
        ZoneScopedN(TracyFunction);
        if (value.empty()) return std::nullopt;

        T parsed_number = 0;
        std::from_chars_result result = std::from_chars(value.begin(), value.end(), parsed_number);

        if (result.ec != std::errc()) return std::nullopt;
        for (T t: list) {
            if (parsed_number == t) return parsed_number;
        }

        return std::nullopt;
    }

    constexpr std::optional<MelonDsDs::BootMode> ParseBootMode(std::string_view value) noexcept {
        if (value == config::values::NATIVE) return BootMode::Native;
        if (value == config::values::DIRECT) return BootMode::Direct;
        return std::nullopt;
    }

    constexpr std::optional<MelonDsDs::SysfileMode> ParseSysfileMode(std::string_view value) noexcept {
        if (value == config::values::NATIVE) return SysfileMode::Native;
        if (value == config::values::BUILT_IN) return SysfileMode::BuiltIn;
        return std::nullopt;
    }

    constexpr std::optional<MelonDsDs::AlarmMode> ParseAlarmMode(std::string_view value) noexcept {
        if (value == config::values::DISABLED) return AlarmMode::Disabled;
        if (value == config::values::ENABLED) return AlarmMode::Enabled;
        if (value == config::values::DEFAULT) return AlarmMode::Default;
        return std::nullopt;
    }

    constexpr std::optional<MelonDsDs::UsernameMode> ParseUsernameMode(std::string_view value) noexcept {
        if (value.empty() || value == config::values::firmware::DEFAULT_USERNAME) return UsernameMode::MelonDSDS;
        if (value == config::values::firmware::FIRMWARE_USERNAME) return UsernameMode::Firmware;
        if (value == config::values::firmware::GUESS_USERNAME) return UsernameMode::Guess;
        return std::nullopt;
    }

    constexpr std::optional<MelonDsDs::Renderer> ParseRenderer(std::string_view value) noexcept {
        if (value == config::values::SOFTWARE) return MelonDsDs::Renderer::Software;
        if (value == config::values::OPENGL) return MelonDsDs::Renderer::OpenGl;
        return std::nullopt;
    }

    constexpr std::optional<MelonDsDs::CursorMode> ParseCursorMode(std::string_view value) noexcept {
        if (value == config::values::DISABLED) return MelonDsDs::CursorMode::Never;
        if (value == config::values::TOUCHING) return MelonDsDs::CursorMode::Touching;
        if (value == config::values::TIMEOUT) return MelonDsDs::CursorMode::Timeout;
        if (value == config::values::ALWAYS) return MelonDsDs::CursorMode::Always;
        return std::nullopt;
    }

    constexpr std::optional<MelonDsDs::ConsoleType> ParseConsoleType(std::string_view value) noexcept {
        if (value == config::values::DS) return MelonDsDs::ConsoleType::DS;
        if (value == config::values::DSI) return MelonDsDs::ConsoleType::DSi;
        return std::nullopt;
    }

    constexpr std::optional<MelonDsDs::NetworkMode> ParseNetworkMode(std::string_view value) noexcept {
        if (value == config::values::DISABLED) return MelonDsDs::NetworkMode::None;
        if (value == config::values::DIRECT) return MelonDsDs::NetworkMode::Direct;
        if (value == config::values::INDIRECT) return MelonDsDs::NetworkMode::Indirect;
        return std::nullopt;
    }

    constexpr std::optional<MelonDsDs::ScreenLayout> ParseScreenLayout(std::string_view value) noexcept {
        using MelonDsDs::ScreenLayout;
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
        using MelonDsDs::ScreenLayout;
        if (value == config::values::ONE) return MelonDsDs::HybridSideScreenDisplay::One;
        if (value == config::values::BOTH) return MelonDsDs::HybridSideScreenDisplay::Both;

        return std::nullopt;
    }

    constexpr std::optional<FirmwareLanguage> ParseLanguage(std::string_view value) noexcept {
        if (value == config::values::AUTO) return MelonDsDs::FirmwareLanguage::Auto;
        if (value == config::values::DEFAULT) return MelonDsDs::FirmwareLanguage::Default;
        if (value == config::values::JAPANESE) return MelonDsDs::FirmwareLanguage::Japanese;
        if (value == config::values::ENGLISH) return MelonDsDs::FirmwareLanguage::English;
        if (value == config::values::FRENCH) return MelonDsDs::FirmwareLanguage::French;
        if (value == config::values::GERMAN) return MelonDsDs::FirmwareLanguage::German;
        if (value == config::values::ITALIAN) return MelonDsDs::FirmwareLanguage::Italian;
        if (value == config::values::SPANISH) return MelonDsDs::FirmwareLanguage::Spanish;

        return std::nullopt;
    }

    constexpr std::optional<MicInputMode> ParseMicInputMode(std::string_view value) noexcept {
        if (value == config::values::MICROPHONE) return MicInputMode::HostMic;
        if (value == config::values::NOISE) return MicInputMode::WhiteNoise;
        if (value == config::values::SILENCE) return MicInputMode::None;

        return std::nullopt;
    }

    constexpr std::optional<MicButtonMode> ParseMicButtonMode(std::string_view value) noexcept {
        if (value == config::values::HOLD) return MicButtonMode::Hold;
        if (value == config::values::TOGGLE) return MicButtonMode::Toggle;
        if (value == config::values::ALWAYS) return MicButtonMode::Always;

        return std::nullopt;
    }

    constexpr std::optional<MelonDsDs::TouchMode> ParseTouchMode(std::string_view value) noexcept {
        if (value == config::values::AUTO) return TouchMode::Auto;
        if (value == config::values::TOUCH) return TouchMode::Pointer;
        if (value == config::values::JOYSTICK) return TouchMode::Joystick;

        return std::nullopt;
    }

    constexpr std::optional<melonDS::AudioBitDepth> ParseBitDepth(std::string_view value) noexcept {
        if (value == config::values::_10BIT) return melonDS::AudioBitDepth::_10Bit;
        if (value == config::values::_16BIT) return melonDS::AudioBitDepth::_16Bit;
        if (value == config::values::AUTO) return melonDS::AudioBitDepth::Auto;

        return std::nullopt;
    }

    constexpr std::optional<melonDS::AudioInterpolation> ParseInterpolation(std::string_view value) noexcept {
        if (value == config::values::CUBIC) return melonDS::AudioInterpolation::Cubic;
        if (value == config::values::COSINE) return melonDS::AudioInterpolation::Cosine;
        if (value == config::values::LINEAR) return melonDS::AudioInterpolation::Linear;
        if (value == config::values::DISABLED) return melonDS::AudioInterpolation::None;

        return std::nullopt;
    }

    constexpr std::optional<ScreenFilter> ParseScreenFilter(std::string_view value) noexcept {
        if (value == config::values::LINEAR) return ScreenFilter::Linear;
        if (value == config::values::NEAREST) return ScreenFilter::Nearest;

        return std::nullopt;
    }

    std::optional<melonDS::IpAddress> ParseIpAddress(std::string_view value) noexcept;
}
#endif // MELONDSDS_PARSE_HPP
