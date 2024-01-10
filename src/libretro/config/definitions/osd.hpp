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

#ifndef MELONDS_DS_OSD_HPP
#define MELONDS_DS_OSD_HPP

#include <libretro.h>

#include "../constants.hpp"

namespace MelonDsDs::config::definitions {
    constexpr retro_core_option_v2_definition ShowUnsupportedFeatures {
        config::osd::UNSUPPORTED_FEATURES,
        "Warn About Unsupported Features",
        nullptr,
        "Enable to display an on-screen message "
        "if melonDS tries to use certain unsupported features. "
        "These warnings will be logged regardless of this setting.",
        nullptr,
        config::osd::CATEGORY,
        {
            {MelonDsDs::config::values::ENABLED, nullptr},
            {MelonDsDs::config::values::DISABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::ENABLED
    };

    constexpr retro_core_option_v2_definition ShowBiosWarnings {
        config::osd::BIOS_WARNINGS,
        "Warn About Certain BIOS Problems",
        nullptr,
        "Enable to display warnings if your BIOS files have certain known problems. "
        "A warning will always be shown if your BIOS files are missing or invalid, "
        "regardless of this setting.",
        nullptr,
        config::osd::CATEGORY,
        {
            {MelonDsDs::config::values::ENABLED, nullptr},
            {MelonDsDs::config::values::DISABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::ENABLED
    };

    constexpr retro_core_option_v2_definition ShowCurrentLayout {
        config::osd::CURRENT_LAYOUT,
        "Show Screen Layout",
        nullptr,
        "Enable to show which screen layout within the configured sequence is active.",
        nullptr,
        config::osd::CATEGORY,
        {
            {MelonDsDs::config::values::ENABLED, nullptr},
            {MelonDsDs::config::values::DISABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::ENABLED
    };

    constexpr retro_core_option_v2_definition ShowMicState {
        config::osd::MIC_STATE,
        "Show Host Microphone State",
        nullptr,
        "Enable to show whether your device's microphone is active.",
        nullptr,
        config::osd::CATEGORY,
        {
            {MelonDsDs::config::values::ENABLED, nullptr},
            {MelonDsDs::config::values::DISABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::ENABLED
    };

    constexpr retro_core_option_v2_definition ShowCameraState {
        config::osd::CAMERA_STATE,
        "Show Host Camera State",
        nullptr,
        "Enable to show whether your device's camera is active.",
        nullptr,
        config::osd::CATEGORY,
        {
            {MelonDsDs::config::values::ENABLED, nullptr},
            {MelonDsDs::config::values::DISABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::ENABLED
    };

    constexpr retro_core_option_v2_definition ShowLidState {
        config::osd::LID_STATE,
        "Show Lid State",
        nullptr,
        "Enable to show whether the emulated screens are closed.",
        nullptr,
        config::osd::CATEGORY,
        {
            {MelonDsDs::config::values::ENABLED, nullptr},
            {MelonDsDs::config::values::DISABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::ENABLED
    };

#ifndef NDEBUG
    constexpr retro_core_option_v2_definition ShowPointerCoordinates {
        config::osd::POINTER_COORDINATES,
        "Show Pointer Coordinates",
        nullptr,
        "Enable to display the coordinates of the pointer within the touch screen. "
        "Used for debugging. "
        "Leave disabled if unsure.",
        nullptr,
        config::osd::CATEGORY,
        {
            {MelonDsDs::config::values::ENABLED, nullptr},
            {MelonDsDs::config::values::DISABLED, nullptr},
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::DISABLED
    };
#endif

    constexpr std::initializer_list<retro_core_option_v2_definition> OsdOptionDefinitions {
        ShowUnsupportedFeatures,
        ShowBiosWarnings,
        ShowCurrentLayout,
        ShowMicState,
        ShowCameraState,
        ShowLidState,
#ifndef NDEBUG
        ShowPointerCoordinates,
#endif
    };
}

#endif //MELONDS_DS_OSD_HPP
