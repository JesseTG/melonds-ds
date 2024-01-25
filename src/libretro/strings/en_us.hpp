/*
    Copyright 2024 Jesse Talavera

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

#ifndef MELONDSDS_STRINGS_EN_US_HPP
#define MELONDSDS_STRINGS_EN_US_HPP

// These strings are intended to be shown directly to the player;
// log messages don't need to be declared here.
namespace MelonDsDs::strings::en_us {
    constexpr const char* const InternalError =
        "An internal error occurred with melonDS DS. "
        "Please contact the developer with the log file.";

    constexpr const char* const UnknownError =
        "An unknown error has occurred with melonDS DS. "
        "Please contact the developer with the log file.";

    constexpr const char* const InvalidCheat = "Cheat #{} ({:.8}...) isn't valid, ignoring it.";

    constexpr const char* const ArchivedGbaSaveNotSupported =
        "melonDS DS does not support archived GBA save data right now. "
        "Please extract it and try again. "
        "Continuing without using the save data.";

    constexpr const char* const CantDisableCheat =
        "Action Replay codes can't be undone, restart the game to disable them.";

    constexpr const char* const CompressedGbaSaveNotSupported =
        "melonDS DS does not support compressed GBA save data right now. "
        "Please disable save data compression in the frontend and try again. "
        "Continuing without using the save data.";

    constexpr const char* const DsiDoesntHaveGbaSlot =
        "The DSi doesn't have a GBA slot, "
        "please use DS mode instead. "
        "Ignoring the loaded GBA ROM.";

    constexpr const char* const GbaModeNotSupported =
        "GBA mode is not supported. Use a GBA core instead.";

    constexpr const char* const IndirectWifiInitFailed =
        "Failed to initialize indirect-mode Wi-fi support. Wi-fi will be disabled.";

    constexpr const char* const InternalConsoleError =
        "An internal error occurred in the emulated console.";

    constexpr const char* const HackedFirmwareWarning =
        "Corrupted firmware detected! "
        "Any game that alters Wi-fi settings will break this firmware, even on real hardware.";

    constexpr const char* const MicNotSupported =
        "This frontend doesn't support microphones.";

    constexpr const char* const OpenGlInitFailed =
        "Failed to initialize OpenGL, falling back to software mode.";

    constexpr const char* const OptionInitFailed =
        "Failed to set core option definitions, functionality will be limited.";

    constexpr const char* const PleaseResetCore =
        "Please follow the advice on this screen, then unload/reload the core.";

    constexpr const char* const ScreenRotateFailed =
        "Failed to rotate screen.";

    constexpr const char* const StateTooOld =
        "This savestate is too old, can't load it.\n"
        "Save your game normally in the older version and import the save data.";

    constexpr const char* const StateTooNew =
        "This savestate is too new, can't load it.\n"
        "Save your game normally in the newer version, "
        "then update this core or import the save data.";

    constexpr const char* const StateLoadFailed =
        "Can't load this savestate; did it come from the right core and game?";

    constexpr const char* const SysDirFailed =
        "Failed to get the system directory, functionality will be limited.";

    constexpr const char* const CurrentLayout = "{}Layout {}/{}";
    constexpr const char* const ScreenState = "{}Closed";
}

#endif // MELONDSDS_STRINGS_EN_US_HPP
