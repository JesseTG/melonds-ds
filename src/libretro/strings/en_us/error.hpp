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
        "The firmware file at \"{}\" can't be used to boot to the DS menu.";

    constexpr const char* const BuiltInFirmwareNotBootableProblem =
        "The firmware file at \"{}\" can't be used to boot to the DS menu.";

    constexpr const char* const FirmwareNotBootableSolution =
        "Ensure you have native DS (not DSi) firmware in your frontend's system folder. "
        "Pick it in the core options, then restart the core. "
        "If you just want to play a DS game, try setting Boot Mode to \"Direct\" "
        "or BIOS/Firmware Mode to \"Built-In\" in the core options.";
}

#endif //MELONDSDS_STRINGS_EN_US_ERROR_HPP
