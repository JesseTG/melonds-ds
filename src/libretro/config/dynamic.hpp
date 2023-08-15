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

#ifndef MELONDS_DS_DYNAMIC_HPP
#define MELONDS_DS_DYNAMIC_HPP

#include <optional>
#include <string>
#include <vector>

#include <libretro.h>

struct NDSHeader;

namespace melonds::config {
    struct CoreOption {
        std::string value;
        std::string label;

        CoreOption(const std::string &value, const std::string &label) noexcept: value(value), label(label) {}
    };

    class DynamicCoreOptions {
    public:
        DynamicCoreOptions(
            const std::optional<retro_game_info> &nds_info,
            const std::optional<NDSHeader> &nds_header,
            const retro_core_option_v2_definition *definitions,
            size_t definitions_length,
            const retro_core_option_v2_category *categories,
            size_t categories_length
        );

        ~DynamicCoreOptions() noexcept;

        const retro_core_option_v2_definition* GetDefinitions() const noexcept {
            return _optionDefs;
        }

        retro_core_option_v2_definition* GetDefinitions() noexcept {
            return _optionDefs;
        }

        const retro_core_options_v2* GetOptions() const noexcept {
            return &_options;
        }

        retro_core_options_v2* GetOptions() noexcept {
            return &_options;
        }

    private:
        retro_core_option_v2_definition *_optionDefs;
        retro_core_option_v2_definition *_optionDefsEnd;
        retro_core_option_v2_category *_optionCategories;
        size_t _optionDefsLength; // Excluding the null option at the end

        retro_core_options_v2 _options;
        std::vector<std::string> _dsiNandPaths;
    };

}
#endif //MELONDS_DS_DYNAMIC_HPP
