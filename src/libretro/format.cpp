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

#include "format.hpp"
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include "PlatformOGLPrivate.h"
#endif

using namespace melonDS;
using FirmwareConsoleType = Firmware::FirmwareConsoleType;
using DSi_NAND::ConsoleRegion;

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
auto fmt::formatter<MelonDsDs::FormattedGLEnum>::format(MelonDsDs::FormattedGLEnum e, format_context& ctx) const -> decltype(ctx.out()) {
    string_view name = "<unknown>";
    switch ((int)e) {
        case GL_NO_ERROR:
            name = "GL_NO_ERROR";
            break;
        case GL_INVALID_VALUE:
            name = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            name = "GL_INVALID_OPERATION";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            name = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            name = "GL_OUT_OF_MEMORY";
            break;
        case GL_INVALID_ENUM:
            name = "GL_INVALID_ENUM";
            break;
        case GL_FRAMEBUFFER_COMPLETE:
            name = "GL_FRAMEBUFFER_COMPLETE";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            name = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            name = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            name = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            name = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            name = "GL_FRAMEBUFFER_UNSUPPORTED";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            name = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            name = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
            break;
    }
    return formatter<string_view>::format(name, ctx);
}
#endif

auto fmt::formatter<MelonDsDs::BiosType>::format(MelonDsDs::BiosType c, format_context& ctx) const -> decltype(ctx.out()) {
    string_view name = "unknown";
    switch (c) {
        case MelonDsDs::BiosType::Arm7:
            name = "ARM7";
            break;
        case MelonDsDs::BiosType::Arm9:
            name = "ARM9";
            break;
        case MelonDsDs::BiosType::Arm7i:
            name = "DSi ARM7";
            break;
        case MelonDsDs::BiosType::Arm9i:
            name = "DSi ARM9";
            break;
    }
    return formatter<string_view>::format(name, ctx);
}

auto fmt::formatter<Platform::StopReason>::format(Platform::StopReason c, format_context& ctx) const -> decltype(ctx.out()) {
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

auto fmt::formatter<FirmwareConsoleType>::format(FirmwareConsoleType c, format_context& ctx) const -> decltype(ctx.out()) {
    string_view name = "unknown";
    switch (c) {
        case FirmwareConsoleType::DS:
            name = "DS";
            break;
        case FirmwareConsoleType::DSLite:
            name = "DS Lite";
            break;
        case FirmwareConsoleType::DSi:
            name = "DSi";
            break;
        case FirmwareConsoleType::iQueDS:
            name = "iQue DS";
            break;
        case FirmwareConsoleType::iQueDSLite:
            name = "iQueue DS Lite";
            break;
    }
    return formatter<string_view>::format(name, ctx);
}

auto fmt::formatter<MelonDsDs::ConsoleType>::format(MelonDsDs::ConsoleType c, format_context& ctx) const -> decltype(ctx.out()) {
    string_view name = "unknown";
    switch (c) {
        case MelonDsDs::ConsoleType::DS:
            name = "DS";
            break;
        case MelonDsDs::ConsoleType::DSi:
            name = "DSi";
            break;
    }
    return formatter<string_view>::format(name, ctx);
}

auto fmt::formatter<ConsoleRegion>::format(ConsoleRegion c, format_context& ctx) const -> decltype(ctx.out()) {
    string_view name = "<unknown>";
    switch (c) {
        case ConsoleRegion::Japan:
            name = "Japan";
            break;
        case ConsoleRegion::USA:
            name = "USA";
            break;
        case ConsoleRegion::Europe:
            name = "Europe";
            break;
        case ConsoleRegion::Australia:
            name = "Australia";
            break;
        case ConsoleRegion::China:
            name = "China";
            break;
        case ConsoleRegion::Korea:
            name = "Korea";
            break;
    }
    return formatter<string_view>::format(name, ctx);
}


auto fmt::formatter<Firmware::Language>::format(Firmware::Language c, format_context& ctx) const -> decltype(ctx.out()) {
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
        case melonDS::Firmware::Language::Italian:
            name = "Italian";
            break;
        case melonDS::Firmware::Language::Spanish:
            name = "Spanish";
            break;
        case melonDS::Firmware::Language::Chinese:
            name = "Chinese";
            break;
        case melonDS::Firmware::Language::Reserved:
            name = "Reserved";
            break;
    }
    return formatter<string_view>::format(name, ctx);
}


auto fmt::formatter<retro_language>::format(retro_language lang, format_context& ctx) const -> decltype(ctx.out()) {
    string_view name;
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
        default:
            name = "<unknown>";
            break;
    }
    return formatter<string_view>::format(name, ctx);
}

auto fmt::formatter<RegionMask>::format(RegionMask mask, format_context& ctx) const -> decltype(ctx.out()) {
    std::vector<string_view> regions;
    if (mask == melonDS::RegionMask::RegionFree) {
        regions.push_back("RegionFree");
    }
    else {
        if (mask & melonDS::RegionMask::Japan)
            regions.push_back("Japan");

        if (mask & melonDS::RegionMask::USA)
            regions.push_back("USA");

        if (mask & melonDS::RegionMask::Europe)
            regions.push_back("Europe");

        if (mask & melonDS::RegionMask::Australia)
            regions.push_back("Australia");

        if (mask & melonDS::RegionMask::China)
            regions.push_back("China");

        if (mask & melonDS::RegionMask::Korea)
            regions.push_back("Korea");

        if (mask & melonDS::RegionMask::Reserved)
            regions.push_back("Reserved");
    }

    return formatter<decltype(regions)>::format(regions, ctx);
}

auto fmt::formatter<scaler_pix_fmt>::format(scaler_pix_fmt c, format_context& ctx) const -> decltype(ctx.out()) {
    string_view name;
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
        case SCALER_FMT_BGR24:
            name = "BGR24";
            break;
        case SCALER_FMT_YUYV:
            name = "YUYV";
            break;
        case SCALER_FMT_RGBA4444:
            name = "RGBA4444";
            break;
        default:
            name = "<unknown>";
            break;
    }
    return formatter<string_view>::format(name, ctx);
}

auto fmt::formatter<Platform::FileMode>::format(Platform::FileMode mode, format_context& ctx) const -> decltype(ctx.out()) {
    std::vector<std::string_view> bits;

    if (mode & Platform::FileMode::Read)
        bits.emplace_back("Read");

    if (mode & Platform::FileMode::Write)
        bits.emplace_back("Write");

    if (mode & Platform::FileMode::Preserve)
        bits.emplace_back("Preserve");

    if (mode & Platform::FileMode::NoCreate)
        bits.emplace_back("NoCreate");

    if (mode & Platform::FileMode::Text)
        bits.emplace_back("Text");

    return formatter<std::vector<std::string_view>>::format(bits, ctx);
}
