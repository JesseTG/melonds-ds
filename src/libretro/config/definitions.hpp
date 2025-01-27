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

#ifndef MELONDS_DS_DEFINITIONS_HPP
#define MELONDS_DS_DEFINITIONS_HPP

#include <array>

#include <libretro.h>

#include "config/definitions/audio.hpp"
#include "config/definitions/cpu.hpp"
#include "config/definitions/firmware.hpp"
#include "config/definitions/network.hpp"
#include "config/definitions/osd.hpp"
#include "config/definitions/screen.hpp"
#include "config/definitions/system.hpp"
#include "config/definitions/time.hpp"
#include "config/definitions/video.hpp"

// All descriptive text uses semantic line breaks. https://sembr.org

namespace MelonDsDs::config::definitions {
    constexpr std::array CoreOptionDefinitions {
        MicInput,
        MicInputButton,
        BitDepth,
        AudioInterpolation,

#ifdef JIT_ENABLED
        JitEnabled,
        JitBlockSize,
        JitBranchOptimizations,
        JitLiteralOptimizations,
#   ifdef HAVE_JIT_FASTMEM
        JitFastMemory,
#   endif
#endif

        LanMacAddressMode,
#ifdef HAVE_NETWORKING
        NetworkMode,
#   ifdef HAVE_NETWORKING_DIRECT_MODE
        NetworkInterface,
#   endif
#endif

        ShowCursor,
        CursorTimeout,
        TouchMode,
        NumberOfScreenLayouts,
        ScreenLayout1,
        ScreenLayout2,
        ScreenLayout3,
        ScreenLayout4,
        ScreenLayout5,
        ScreenLayout6,
        ScreenLayout7,
        ScreenLayout8,
        HybridRatio,
        HybridSmallScreen,
        HybridScreenFiltering,
        ScreenGap,

        DnsOverride,
        Language,
        Username,
        FavoriteColor,
        BirthMonth,
        BirthDay,
        EnableAlarm,
        AlarmHour,
        AlarmMinute,

        ConsoleMode,
        Slot2Device,
        SolarSensorMode,
        SysfileMode,
        FirmwarePath,
        DsiFirmwarePath,
        NandPath,
        BootMode,
        DsiSdCardSaveMode,
        DsiSdCardReadOnly,
        DsiSdCardSyncToHost,
        HomebrewSdCard,
        HomebrewSdCardReadOnly,
        HomebrewSdCardSyncToHost,
        BatteryUpdateInterval,
        NdsPowerOkThreshold,

        StartTimeMode,
        RelativeYearOffset,
        RelativeDayOffset,
        RelativeHourOffset,
        RelativeMinuteOffset,
        AbsoluteYear,
        AbsoluteMonth,
        AbsoluteDay,
        AbsoluteHour,
        AbsoluteMinute,

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
        RenderMode,
        OpenGlScaleFactor,
        OpenGlBetterPolygons,
#endif
#if defined(HAVE_THREADS) && defined(HAVE_THREADED_RENDERER)
        ThreadedSoftwareRenderer,
#endif

        ShowUnsupportedFeatures,
        ShowBiosWarnings,
        ShowCurrentLayout,
        ShowMicState,
        ShowCameraState,
        ShowLidState,
        ShowSensorReading,
#ifndef NDEBUG
        ShowPointerCoordinates,
#endif
        retro_core_option_v2_definition {},
    };
}
#endif //MELONDS_DS_DEFINITIONS_HPP
