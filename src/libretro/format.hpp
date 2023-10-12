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


#ifndef MELONDS_DS_FORMAT_HPP
#define MELONDS_DS_FORMAT_HPP

#include <string_view>

#include <fmt/core.h>
#include <fmt/format.h>
#include <SPI_Firmware.h>
#include <Platform.h>

#include "config.hpp"

template<>
struct fmt::formatter<melonds::BiosType> : fmt::formatter<std::string_view> {
    // use inherited 'formatter<string_view>::parse'…
    // … and only implement 'format':
    template<typename FmtContext>
    auto format(melonds::BiosType c, FmtContext& ctx) {
        string_view name = "unknown";
        switch (c) {
            case melonds::BiosType::Arm7:
                name = "ARM7";
                break;
            case melonds::BiosType::Arm9:
                name = "ARM9";
                break;
            case melonds::BiosType::Arm7i:
                name = "DSi ARM7";
                break;
            case melonds::BiosType::Arm9i:
                name = "DSi ARM9";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template<>
struct fmt::formatter<Platform::StopReason> : fmt::formatter<std::string_view> {
    // use inherited 'formatter<string_view>::parse'…
    // … and only implement 'format':
    template<typename FmtContext>
    auto format(Platform::StopReason c, FmtContext& ctx) {
        string_view name = "???";
        switch (c) {
            case Platform::StopReason::Unknown:
                name = "Unknown";
                break;
            case Platform::StopReason::External:
                name = "External";
                break;
            case Platform::StopReason::GBAModeNotSupported:
                name = "GBAModeNotSupported";
                break;
            case Platform::StopReason::BadExceptionRegion:
                name = "BadExceptionRegion";
                break;
            case Platform::StopReason::PowerOff:
                name = "PowerOff";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template<>
struct fmt::formatter<SPI_Firmware::FirmwareConsoleType> : fmt::formatter<std::string_view> {
    // use inherited 'formatter<string_view>::parse'…
    // … and only implement 'format':
    template<typename FmtContext>
    auto format(SPI_Firmware::FirmwareConsoleType c, FmtContext& ctx) {
        string_view name = "unknown";
        switch (c) {
            case SPI_Firmware::FirmwareConsoleType::DS:
                name = "DS";
                break;
            case SPI_Firmware::FirmwareConsoleType::DSLite:
                name = "DS Lite";
                break;
            case SPI_Firmware::FirmwareConsoleType::DSi:
                name = "DSi";
                break;
            case SPI_Firmware::FirmwareConsoleType::iQueDS:
                name = "iQue DS";
                break;
            case SPI_Firmware::FirmwareConsoleType::iQueDSLite:
                name = "iQueue DS Lite";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template<>
struct fmt::formatter<melonds::ConsoleType> : fmt::formatter<std::string_view> {
    // use inherited 'formatter<string_view>::parse'…
    // … and only implement 'format':
    template<typename FmtContext>
    auto format(melonds::ConsoleType c, FmtContext& ctx) {
        string_view name = "unknown";
        switch (c) {
            case melonds::ConsoleType::DS:
                name = "DS";
                break;
            case melonds::ConsoleType::DSi:
                name = "DSi";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

#endif //MELONDS_DS_FORMAT_HPP
