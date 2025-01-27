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

#include <libretro.h>

#include "../constants.hpp"

namespace MelonDsDs::config::definitions {
    constexpr retro_core_option_v2_definition MicInput {
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
            {MelonDsDs::config::values::SILENCE, "Silence"},
            {MelonDsDs::config::values::BLOW, "Blow"},
            {MelonDsDs::config::values::NOISE, "Noise"},
            {MelonDsDs::config::values::MICROPHONE, "Microphone"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::MICROPHONE,
    };

    constexpr retro_core_option_v2_definition MicInputButton {
        config::audio::MIC_INPUT_BUTTON,
        "Microphone Button Mode",
        nullptr,
        "Set the behavior of the Microphone button, "
        "even if Microphone Input Mode is set to Blow or Noise. "
        "The emulated microphone receives silence when disabled by the button.\n"
        "\n"
        "Hold: Button enables mic input while held.\n"
        "Toggle: Button enables mic input when pressed, disables it when pressed again.\n"
        "Always: Button is ignored, mic input is always enabled.\n"
        "\n"
        "Ignored if Microphone Input Mode is set to Silence.",
        nullptr,
        config::audio::CATEGORY,
        {
            {MelonDsDs::config::values::HOLD, "Hold"},
            {MelonDsDs::config::values::TOGGLE, "Toggle"},
            {MelonDsDs::config::values::ALWAYS, "Always"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::HOLD,
    };

    constexpr retro_core_option_v2_definition BitDepth{
        config::audio::AUDIO_BITDEPTH,
        "Audio Bit Depth",
        "Bit Depth",
        "The number of bits used for each audio sample. "
        "Automatic matches the original hardware's behavior "
        "by using 10-bit audio for DS mode "
        "and 16-bit audio for DSi mode. "
        "If unsure, set to Automatic.",
        nullptr,
        config::audio::CATEGORY,
        {
            {MelonDsDs::config::values::AUTO, "Automatic"},
            {MelonDsDs::config::values::_10BIT, "10-bit"},
            {MelonDsDs::config::values::_16BIT, "16-bit"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::AUTO
    };

    constexpr retro_core_option_v2_definition AudioInterpolation {
        config::audio::AUDIO_INTERPOLATION,
        "Audio Interpolation",
        "Interpolation",
        "Interpolates audio output for improved quality. "
        "Disable this to match the behavior of the original DS hardware.",
        nullptr,
        config::audio::CATEGORY,
        {
            {MelonDsDs::config::values::DISABLED, nullptr},
            {MelonDsDs::config::values::LINEAR, "Linear"},
            {MelonDsDs::config::values::COSINE, "Cosine"},
            {MelonDsDs::config::values::CUBIC, "Cubic"},
            {MelonDsDs::config::values::GAUSSIAN, "Gaussian"},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::DISABLED
    };

    constexpr std::initializer_list<retro_core_option_v2_definition> AudioOptionDefinitions {
        MicInput,
        MicInputButton,
        BitDepth,
        AudioInterpolation,
    };
}
#endif //MELONDS_DS_AUDIO_HPP
