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

#include "dynamic.hpp"
#include "constants.hpp"
#include "environment.hpp"
#include "retro/dirent.hpp"
#include "tracy.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

#include <retro_dirent.h>
#include <retro_assert.h>
#include <string/stdstring.h>
#include <vfs/vfs_implementation.h>

using std::find_if;
using std::min;
using std::optional;
using std::pair;
using std::string;
using std::string_view;
using std::vector;

constexpr size_t DSI_NAND_SIZE = 251658304;

static vector<string> GetNandPaths() noexcept;

melonds::config::DynamicCoreOptions::DynamicCoreOptions(
        const retro_core_option_v2_definition *definitions,
        size_t definitions_length,
        const retro_core_option_v2_category *categories,
        size_t categories_length
) {
    ZoneScopedN("melonds::config::DynamicCoreOptions::DynamicCoreOptions");
    // TODO: Generate an option for selecting the wi-fi interface
    // TODO: Generate an option for selecting the DS BIOS7 file
    // TODO: Generate an option for selecting the DS BIOS9 file
    // TODO: Generate an option for selecting the DS firmware file
    // TODO: Generate an option for selecting the DSi BIOS7 file
    // TODO: Generate an option for selecting the DSi BIOS9 file
    // TODO: Generate an option for selecting the DSi firmware file
    // TODO: Generate an option for selecting the homebrew SD card
    // TODO: Generate an option for selecting the DSi SD card
    _optionDefs = new retro_core_option_v2_definition[definitions_length];
    _optionDefsEnd = &_optionDefs[definitions_length - 1];
    _optionDefsLength = definitions_length;
    memcpy(_optionDefs, definitions, definitions_length * sizeof(retro_core_option_v2_definition));

    _optionCategories = new retro_core_option_v2_category[categories_length];
    memcpy(_optionCategories, categories, categories_length * sizeof(retro_core_option_v2_category));



    {
        retro_core_option_v2_definition* dsiNandPathOption = find_if(_optionDefs, _optionDefsEnd, [](const auto& def) {
            return string_is_equal(def.key, melonds::config::storage::DSI_NAND_PATH);
        });

        retro_assert(dsiNandPathOption != _optionDefsEnd);

        for (const string& path : GetNandPaths()) {
            _dsiNandPaths.push_back(std::move(path));
        }

        if (!_dsiNandPaths.empty()) {
            memset(dsiNandPathOption->values, 0, sizeof(dsiNandPathOption->values));
            int length = min((int)_dsiNandPaths.size(), (int)RETRO_NUM_CORE_OPTION_VALUES_MAX - 1);
            for (int i = 0; i < length; ++i) {
                dsiNandPathOption->values[i].value = _dsiNandPaths[i].c_str();
                dsiNandPathOption->values[i].label = nullptr;
            }

            dsiNandPathOption->default_value = _dsiNandPaths[0].c_str();
        }
    }

    _options.categories = _optionCategories;
    _options.definitions = _optionDefs;
}

melonds::config::DynamicCoreOptions::~DynamicCoreOptions() noexcept {
    delete[] _optionCategories;
    delete[] _optionDefs;
}

static vector<string> GetNandPaths() noexcept {
    vector<string> paths;

    auto appendPaths = [&paths](optional<string> base) {
        if (base) {
            for (const retro::dirent& d : retro::readdir(*base, true)) {
                retro_assert(retro::is_regular_file(d.flags));
                if (d.size == DSI_NAND_SIZE) {
                    // If this is a regular file...
                    string_view path = d.path;
                    retro_assert(path.size() > base->size());

                    path.remove_prefix(base->size() + 1);
                    paths.emplace_back(path);
                }
            }
        }
    };

    appendPaths(retro::get_system_directory());
    appendPaths(retro::get_system_subdirectory());
    appendPaths(retro::get_system_fallback_subdirectory());

    return paths;
}