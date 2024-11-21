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

#ifndef MELONDSDS_CONFIG_TYPES_HPP
#define MELONDSDS_CONFIG_TYPES_HPP

#include <cstdint>

namespace MelonDsDs {
    enum class ConsoleType {
        DS = 0,
        DSi = 1,
    };

    enum class Slot2Device {
        Auto,
        RumblePak,
        MemoryExpansionPak,
        SolarSensorBoktai1,
        SolarSensorBoktai2,
        SolarSensorBoktai3,
    };

    enum RumbleMotorType {
        None = 0,
        Strong = 0b01,
        Weak = 0b10,
        Both = Strong | Weak,
    };

    enum class BiosType {
        Arm7,
        Arm9,
        Arm7i,
        Arm9i,
    };

    enum class ScreenSwapMode {
        Hold,
        Toggle,
    };

    enum class MicButtonMode {
        Hold,
        Toggle,
        Always,
    };

    enum class TouchMode {
        Auto,
        Pointer,
        Joystick,
    };

    enum class RenderMode {
        Software = 0,
        OpenGl = 1,
    };

    enum class MicInputMode {
        None,
        Blow,
        HostMic,
        WhiteNoise,
    };

    enum class FirmwareLanguage {
        Japanese = 0,
        English = 1,
        French = 2,
        German = 3,
        Italian = 4,
        Spanish = 5,
        Chinese = 6,
        Default = 7,
        Auto = 8,
    };

    enum class AlarmMode {
        Default,
        Enabled,
        Disabled,
    };

    enum class BootMode {
        Direct,
        Native,
    };

    enum class SysfileMode {
        BuiltIn,
        Native,
    };

    enum class UsernameMode {
        MelonDSDS,
        Guess,
        Firmware,
    };

    enum class Color {
        Gray,
        Brown,
        Red,
        LightPink,
        Orange,
        Yellow,
        Lime,
        LightGreen,
        DarkGreen,
        Turquoise,
        LightBlue,
        Blue,
        DarkBlue,
        DarkPurple,
        LightPurple,
        DarkPink,
        Default,
    };

    enum class ScreenFilter {
        Nearest,
        Linear,
    };


    enum class ScreenLayout {
        TopBottom = 0,
        BottomTop = 1,
        LeftRight = 2,
        RightLeft = 3,
        TopOnly = 4,
        BottomOnly = 5,
        HybridTop = 6,
        HybridBottom = 7,
        FlippedHybridTop = 8,
        FlippedHybridBottom = 9,
        TurnLeft = 10,
        TurnRight = 11,
        UpsideDown = 12,
    };

    enum class HybridSideScreenDisplay {
        One,
        Both
    };

    enum class CursorMode {
        Never,
        Touching,
        Timeout,
        Always,
    };

    enum class NetworkMode {
        None,
        Direct,
        Indirect,
    };

    enum class StartTimeMode {
        Real,
        Sync,
        Relative,
        Absolute,
    };

    enum class FormattedGLEnum {};

    enum class FormattedPCapFlags : uint32_t {};
}

#endif // MELONDSDS_CONFIG_TYPES_HPP
