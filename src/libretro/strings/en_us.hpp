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
}

#endif // MELONDSDS_STRINGS_EN_US_HPP
