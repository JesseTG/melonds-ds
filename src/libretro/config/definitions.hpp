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
#include <cstdint>
#include <tuple>

#include <libretro.h>

#include "config/definitions/audio.hpp"
#include "config/definitions/cpu.hpp"
#include "config/definitions/firmware.hpp"
#include "config/definitions/network.hpp"
#include "config/definitions/osd.hpp"
#include "config/definitions/screen.hpp"
#include "config/definitions/system.hpp"
#include "config/definitions/video.hpp"

// All descriptive text uses semantic line breaks. https://sembr.org

// I know this is a monstrosity. The idea is to make it easier to add new options.
namespace MelonDsDs::config::definitions {
    template<retro_language L = RETRO_LANGUAGE_ENGLISH>
    constexpr std::tuple CoreOptionDefinitionGroups {
        AudioOptionDefinitions<L>,
        CpuOptionDefinitions<L>,
        NetworkOptionDefinitions<L>,
        ScreenOptionDefinitions<L>,
        FirmwareOptionDefinitions<L>,
        SystemOptionDefinitions<L>,
        VideoOptionDefinitions<L>,
        OsdOptionDefinitions<L>,
    };

    template<retro_language L = RETRO_LANGUAGE_ENGLISH>
    constexpr size_t CoreOptionCount = std::apply([](auto &&... args) { return (args.size() + ...); }, CoreOptionDefinitionGroups<L>);

    // I gotta be honest, I don't know how this works.
    // GitHub Copilot generated it and I'm too scared to change it.
    template<retro_language L = RETRO_LANGUAGE_ENGLISH>
    constexpr std::array<retro_core_option_v2_definition, CoreOptionCount<L> + 1>
    GetCoreOptionDefinitions(decltype(CoreOptionDefinitionGroups<L>) categories) {
        std::array<retro_core_option_v2_definition, CoreOptionCount<L> + 1> result {};
        std::size_t index = 0;

        std::apply([&](auto &&... args) {
            (([&](auto &&category) {
                for (const retro_core_option_v2_definition &o: category) {
                    result[index] = std::move(o);
                    ++index;
                }
            }(args)), ...);
        }, categories);

        return result;
    }

    template<retro_language L>
    constexpr std::array<retro_core_option_v2_definition, CoreOptionCount<L> + 1> CoreOptionDefinitions = GetCoreOptionDefinitions<L>(CoreOptionDefinitionGroups<L>);

    static_assert(CoreOptionDefinitions<RETRO_LANGUAGE_ENGLISH>[CoreOptionCount<RETRO_LANGUAGE_ENGLISH>].key == nullptr);
}
#endif //MELONDS_DS_DEFINITIONS_HPP
