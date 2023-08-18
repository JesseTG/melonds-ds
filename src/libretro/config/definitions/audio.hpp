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

#ifndef MELONDS_DS_AUDIO_HPP
#define MELONDS_DS_AUDIO_HPP

#include <array>
#include <libretro.h>

#include "../constants.hpp"

namespace melonds::config::definitions {
    template<retro_language L>
    constexpr std::array AudioOptionDefinitions {
        retro_core_option_v2_definition {
            config::audio::MIC_INPUT,
            "Microphone Input Mode",
            nullptr,
            "Choose the sound that the emulated microphone will receive:\n"
            "\n"
            "Silence: No audio input.\n"
            "Blow: Loop a built-in blowing sound.\n"
            "Noise: Random white noise.\n"
            "Microphone: Use your real microphone if available, fall back to Silence if not.",
            nullptr,
            config::audio::CATEGORY,
            {
                {melonds::config::values::SILENCE, "Silence"},
                {melonds::config::values::BLOW, "Blow"},
                {melonds::config::values::NOISE, "Noise"},
                {melonds::config::values::MICROPHONE, "Microphone"},
                {nullptr, nullptr},
            },
            melonds::config::values::MICROPHONE
        },
        retro_core_option_v2_definition {
            config::audio::MIC_INPUT_BUTTON,
            "Microphone Button Mode",
            nullptr,
            "Set the behavior of the Microphone button, "
            "even if Microphone Input Mode is set to Blow or Noise. "
            "The microphone receives silence when disabled by the button.\n"
            "\n"
            "Hold: Button enables mic input while held.\n"
            "Toggle: Button enables mic input when pressed, disables it when pressed again.\n"
            "Always: Button is ignored, mic input is always enabled.\n"
            "\n"
            "Ignored if Microphone Input Mode is set to Silence.",
            nullptr,
            config::audio::CATEGORY,
            {
                {melonds::config::values::HOLD, "Hold"},
                {melonds::config::values::TOGGLE, "Toggle"},
                {melonds::config::values::ALWAYS, "Always"},
                {nullptr, nullptr},
            },
            melonds::config::values::HOLD
        },
        retro_core_option_v2_definition {
            config::audio::AUDIO_BITDEPTH,
            "Audio Bit Depth",
            nullptr,
            "The audio playback bit depth. "
            "Automatic uses 10-bit audio for DS mode "
            "and 16-bit audio for DSi mode.\n"
            "\n"
            "Takes effect at next restart. "
            "If unsure, leave this set to Automatic.",
            nullptr,
            config::audio::CATEGORY,
            {
                {melonds::config::values::AUTO, "Automatic"},
                {melonds::config::values::_10BIT, "10-bit"},
                {melonds::config::values::_16BIT, "16-bit"},
                {nullptr, nullptr},
            },
            melonds::config::values::AUTO
        },
        retro_core_option_v2_definition {
            config::audio::AUDIO_INTERPOLATION,
            "Audio Interpolation",
            nullptr,
            "Interpolates audio output for improved quality. "
            "Disable this to match the behavior of the original DS hardware.",
            nullptr,
            config::audio::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::LINEAR, "Linear"},
                {melonds::config::values::COSINE, "Cosine"},
                {melonds::config::values::CUBIC, "Cubic"},
                {nullptr, nullptr},
            },
            melonds::config::values::DISABLED
        },
    };
}
#endif //MELONDS_DS_AUDIO_HPP
