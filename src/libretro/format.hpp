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
#include <fmt/ranges.h>
#include <SPI_Firmware.h>
#include <DSi_NAND.h>
#include <Platform.h>
#include <libretro.h>
#include <gfx/scaler/scaler.h>

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
struct fmt::formatter<Firmware::FirmwareConsoleType> : fmt::formatter<std::string_view> {
    // use inherited 'formatter<string_view>::parse'…
    // … and only implement 'format':
    template<typename FmtContext>
    auto format(Firmware::FirmwareConsoleType c, FmtContext& ctx) {
        string_view name = "unknown";
        switch (c) {
            case Firmware::FirmwareConsoleType::DS:
                name = "DS";
                break;
            case Firmware::FirmwareConsoleType::DSLite:
                name = "DS Lite";
                break;
            case Firmware::FirmwareConsoleType::DSi:
                name = "DSi";
                break;
            case Firmware::FirmwareConsoleType::iQueDS:
                name = "iQue DS";
                break;
            case Firmware::FirmwareConsoleType::iQueDSLite:
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

template<>
struct fmt::formatter<DSi_NAND::ConsoleRegion> : fmt::formatter<std::string_view> {
    // use inherited 'formatter<string_view>::parse'…
    // … and only implement 'format':
    template<typename FmtContext>
    auto format(DSi_NAND::ConsoleRegion c, FmtContext& ctx) {
        string_view name = "<unknown>";
        switch (c) {
            case DSi_NAND::ConsoleRegion::Japan:
                name = "Japan";
                break;
            case DSi_NAND::ConsoleRegion::USA:
                name = "USA";
                break;
            case DSi_NAND::ConsoleRegion::Europe:
                name = "Europe";
                break;
            case DSi_NAND::ConsoleRegion::Australia:
                name = "Australia";
                break;
            case DSi_NAND::ConsoleRegion::China:
                name = "China";
                break;
            case DSi_NAND::ConsoleRegion::Korea:
                name = "Korea";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template<>
struct fmt::formatter<Firmware::Language> : fmt::formatter<std::string_view> {
    // use inherited 'formatter<string_view>::parse'…
    // … and only implement 'format':
    template<typename FmtContext>
    auto format(Firmware::Language c, FmtContext& ctx) {
        string_view name = "<unknown>";
        switch (c) {
            case Firmware::Language::Japanese:
                name = "Japanese";
                break;
            case Firmware::Language::English:
                name = "English";
                break;
            case Firmware::Language::French:
                name = "French";
                break;
            case Firmware::Language::German:
                name = "German";
                break;
            case Firmware::Language::Italian:
                name = "Italian";
                break;
            case Firmware::Language::Spanish:
                name = "Spanish";
                break;
            case Firmware::Language::Chinese:
                name = "Chinese";
                break;
            case Firmware::Language::Reserved:
                name = "Reserved";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template<>
struct fmt::formatter<retro_language> : fmt::formatter<std::string_view> {
    // use inherited 'formatter<string_view>::parse'…
    // … and only implement 'format':
    template<typename FmtContext>
    auto format(retro_language lang, FmtContext& ctx) {
        string_view name = "<unknown>";
        switch (lang) {
            case RETRO_LANGUAGE_ENGLISH:
                name = "English";
                break;
            case RETRO_LANGUAGE_JAPANESE:
                name = "Japanese";
                break;
            case RETRO_LANGUAGE_FRENCH:
                name = "French";
                break;
            case RETRO_LANGUAGE_SPANISH:
                name = "Spanish";
                break;
            case RETRO_LANGUAGE_GERMAN:
                name = "German";
                break;
            case RETRO_LANGUAGE_ITALIAN:
                name = "Italian";
                break;
            case RETRO_LANGUAGE_DUTCH:
                name = "Dutch";
                break;
            case RETRO_LANGUAGE_PORTUGUESE_BRAZIL:
                name = "Portuguese (Brazil)";
                break;
            case RETRO_LANGUAGE_PORTUGUESE_PORTUGAL:
                name = "Portuguese (Portugal)";
                break;
            case RETRO_LANGUAGE_RUSSIAN:
                name = "Russian";
                break;
            case RETRO_LANGUAGE_KOREAN:
                name = "Korean";
                break;
            case RETRO_LANGUAGE_CHINESE_TRADITIONAL:
                name = "Chinese (Traditional)";
                break;
            case RETRO_LANGUAGE_CHINESE_SIMPLIFIED:
                name = "Chinese (Simplified)";
                break;
            case RETRO_LANGUAGE_ESPERANTO:
                name = "Esperanto";
                break;
            case RETRO_LANGUAGE_POLISH:
                name = "Polish";
                break;
            case RETRO_LANGUAGE_VIETNAMESE:
                name = "Vietnamese";
                break;
            case RETRO_LANGUAGE_ARABIC:
                name = "Arabic";
                break;
            case RETRO_LANGUAGE_GREEK:
                name = "Greek";
                break;
            case RETRO_LANGUAGE_TURKISH:
                name = "Turkish";
                break;
            case RETRO_LANGUAGE_SLOVAK:
                name = "Slovak";
                break;
            case RETRO_LANGUAGE_PERSIAN:
                name = "Persian";
                break;
            case RETRO_LANGUAGE_HEBREW:
                name = "Hebrew";
                break;
            case RETRO_LANGUAGE_ASTURIAN:
                name = "Asturian";
                break;
            case RETRO_LANGUAGE_FINNISH:
                name = "Finnish";
                break;
            case RETRO_LANGUAGE_INDONESIAN:
                name = "Indonesian";
                break;
            case RETRO_LANGUAGE_SWEDISH:
                name = "Swedish";
                break;
            case RETRO_LANGUAGE_UKRAINIAN:
                name = "Ukrainian";
                break;
            case RETRO_LANGUAGE_CZECH:
                name = "Czech";
                break;
            case RETRO_LANGUAGE_CATALAN_VALENCIA:
                name = "Catalan (Valencia)";
                break;
            case RETRO_LANGUAGE_CATALAN:
                name = "Catalan";
                break;
            case RETRO_LANGUAGE_BRITISH_ENGLISH:
                name = "British English";
                break;
            case RETRO_LANGUAGE_HUNGARIAN:
                name = "Hungarian";
                break;
            case RETRO_LANGUAGE_BELARUSIAN:
                name = "Belarusian";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template<>
struct fmt::formatter<RegionMask> : fmt::formatter<std::vector<string_view>> {
    template<typename FmtContext>
    auto format(RegionMask mask, FmtContext& ctx) {
        std::vector<string_view> regions;
        if (mask == RegionMask::RegionFree) {
            regions.push_back("RegionFree");
        } else {
            if (mask & RegionMask::Japan)
                regions.push_back("Japan");

            if (mask & RegionMask::USA)
                regions.push_back("USA");

            if (mask & RegionMask::Europe)
                regions.push_back("Europe");

            if (mask & RegionMask::Australia)
                regions.push_back("Australia");

            if (mask & RegionMask::China)
                regions.push_back("China");

            if (mask & RegionMask::Korea)
                regions.push_back("Korea");

            if (mask & RegionMask::Reserved)
                regions.push_back("Reserved");
        }

        return formatter<decltype(regions)>::format(regions, ctx);
    }
};

template<>
struct fmt::formatter<scaler_pix_fmt> : fmt::formatter<std::string_view> {
    template<typename FmtContext>
    auto format(scaler_pix_fmt c, FmtContext& ctx) {
        string_view name = "<unknown>";
        switch (c) {
            case SCALER_FMT_ARGB8888:
                name = "ARGB8888";
                break;
            case SCALER_FMT_ABGR8888:
                name = "ABGR8888";
                break;
            case SCALER_FMT_0RGB1555:
                name = "0RGB1555";
                break;
            case SCALER_FMT_RGB565:
                name = "RGB565";
                break;
            case SCALER_FMT_RGBA4444:
                name = "RGBA4444";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

#endif //MELONDS_DS_FORMAT_HPP
