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
#include <vector>

#undef isinf
#undef isnan

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <SPI_Firmware.h>
#include <DSi_NAND.h>
#include <Platform.h>
#include <libretro.h>
#include <gfx/scaler/scaler.h>
#include <toml11/fwd/value_t_fwd.hpp>

#include "config/config.hpp"
#include "config/types.hpp"

namespace fmt {
    template<>
    struct formatter<MelonDsDs::FormattedGLEnum> : formatter<std::string_view> {
        auto format(MelonDsDs::FormattedGLEnum e, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<MelonDsDs::FormattedPCapFlags> : formatter<std::vector<string_view>> {
        auto format(MelonDsDs::FormattedPCapFlags e, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<MelonDsDs::BiosType> : formatter<std::string_view> {
        auto format(MelonDsDs::BiosType c, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<melonDS::Platform::StopReason> : formatter<std::string_view> {
        auto format(melonDS::Platform::StopReason c, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<melonDS::Firmware::FirmwareConsoleType> : formatter<std::string_view> {
        auto format(melonDS::Firmware::FirmwareConsoleType c, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<MelonDsDs::ConsoleType> : formatter<std::string_view> {
        auto format(MelonDsDs::ConsoleType c, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<melonDS::DSi_NAND::ConsoleRegion> : formatter<std::string_view> {
        auto format(melonDS::DSi_NAND::ConsoleRegion c, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<melonDS::Firmware::Language> : formatter<std::string_view> {
        auto format(melonDS::Firmware::Language c, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<retro_language> : formatter<std::string_view> {
        auto format(retro_language lang, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<melonDS::RegionMask> : formatter<std::vector<string_view>> {
        auto format(melonDS::RegionMask mask, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<scaler_pix_fmt> : formatter<std::string_view> {
        auto format(scaler_pix_fmt c, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<melonDS::Platform::FileMode> : formatter<std::vector<std::string_view>> {
        auto format(melonDS::Platform::FileMode mode, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<MelonDsDs::ScreenLayout> : formatter<std::string_view> {
        auto format(MelonDsDs::ScreenLayout layout, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<MelonDsDs::RenderMode> : formatter<std::string_view> {
        auto format(MelonDsDs::RenderMode mode, format_context& ctx) const -> decltype(ctx.out());
    };

    template<>
    struct formatter<toml::value_t> : formatter<std::string_view> {
        auto format(toml::value_t type, format_context& ctx) const -> decltype(ctx.out());
    };
}

#endif //MELONDS_DS_FORMAT_HPP
