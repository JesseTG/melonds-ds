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

#ifndef MELONDSDS_STRINGS_EN_US_ERROR_HPP
#define MELONDSDS_STRINGS_EN_US_ERROR_HPP

// For messages that are meant to be displayed on the error screen
namespace MelonDsDs::strings::en_us {
    constexpr const char* const ErrorScreenTitle = "Oh no! melonDS DS couldn't start...";
    constexpr const char* const ErrorScreenSolution = "Here's what you can do:";
    constexpr const char* const ErrorScreenThanks = "Thank you for using melonDS DS!";

    constexpr const char* const NativeFirmwareNotBootableProblem =
        "The firmware file at \"{path}\" can't be used to boot to the DS menu.";

    constexpr const char* const BuiltInFirmwareNotBootableProblem =
        "The built-in firmware can't be used to boot to the DS menu.";

    constexpr const char* const FirmwareNotBootableSolution =
        "Ensure you have native DS (not DSi) firmware in your frontend's system folder. "
        "Pick it in the core options, then restart the core. "
        "If you just want to play a DS game, try setting Boot Mode to \"Direct\" "
        "or BIOS/Firmware Mode to \"Built-In\" in the core options.";

    constexpr const char* const WrongFirmwareProblem =
        "The firmware file at \"{path}\" is for the {firmwareConsole}, but it can't be used in {console} mode.";

    constexpr const char* const WrongFirmwareSolution =
        "Ensure you have a {console}-compatible firmware file in your frontend's system folder (any name works). "
        "Pick it in the core options, then restart the core. "
        "If you just want to play a DS game, try disabling DSi mode in the core options.";

    constexpr const char* const WrongNandRegionProblem =
        "The NAND file at \"{path}\" has the region \"{region}\", "
        "but the loaded DSiWare game will only run in the following regions: {regions}";

    constexpr const char* const WrongNandRegionSolution =
        "Double-check that you're using the right NAND file "
        "and the right copy of your game.";

    constexpr const char* const NoDsiFirmwareProblem =
        "DSi mode requires a firmware file from a DSi, but none was found.";

    constexpr const char* const NoDsiFirmwareSolution =
        "Place your DSi firmware file in your frontend's system folder, "
        "then restart the core. "
        "If you just want to play a DS game, "
        "try disabling DSi mode in the core options.";

    constexpr const char* const NoFirmwareProblem =
        "The core is set to use the firmware file at \"{path}\", but it wasn't there or it couldn't be loaded.";

    constexpr const char* const NoFirmwareSolution =
        "Place your DSi firmware file in your frontend's system folder, name it \"{path}\", then restart the core.";

    constexpr const char* const IncompleteNdsSysfilesProblem =
        "Booting to the native DS menu requires native DS firmware and BIOS files, "
        "but some of them were missing or couldn't be loaded.";

    constexpr const char* const IncompleteNdsSysfilesSolution =
        "Place your DS system files in your frontend's system folder, then restart the core. "
        "If you want to play a regular DS game, try setting Boot Mode to \"Direct\" "
        "and BIOS/Firmware Mode to \"Built-In\" in the core options.";

    constexpr const char* const MissingDsiBiosProblem =
        "DSi mode requires the {bios} BIOS file, but none was found.";

    constexpr const char* const MissingDsiBiosSolution =
        "Place your {bios} BIOS file in your frontend's system folder, name it \"{path}\", then restart the core. "
        "If you want to play a regular DS game, try disabling DSi mode in the core options.";

    constexpr const char* const NoDsiNandProblem =
        "DSi mode requires a NAND image, but none was found.";

    constexpr const char* const NoDsiNandSolution =
        "Place your NAND file in your frontend's system folder (any name works), then restart the core. "
        "If you have multiple NAND files, you can choose one in the core options. "
        "If you want to play a regular DS game, try disabling DSi mode in the core options.";

    constexpr const char* const MissingDsiNandProblem =
        "The core is set to use the NAND file at \"{path}\", but it wasn't there or it couldn't be loaded.";

    constexpr const char* const MissingDsiNandSolution =
        "Place your NAND file in your frontend's system folder, name it \"{path}\", then restart the core. "
        "If you've already done that, ensure that you're using the right NAND file.";

    constexpr const char* const CorruptDsiNandProblem =
        "The core managed to load the configured NAND file at \"{path}\", "
        "but it seems to be corrupted or invalid.";

    constexpr const char* const CorruptDsiNandSolution =
        "Make sure that you're using the right NAND file, "
        "and restore it from a backup copy if necessary. "
        "Check to see if this NAND file works in the standalone melonDS emulator.";
}

#endif //MELONDSDS_STRINGS_EN_US_ERROR_HPP
