/*
    Copyright 2025 Jesse Talavera

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

#include "layout.hpp"

#include "screenlayout.hpp"

using std::optional;

using namespace MelonDsDs;

/*ParsedLayout::ParsedLayout(string_view vfl) {
    // TODO: Populate this metric dict based on config options (except for the ones that should be constant)
    // TODO: Use a hash function that normalizes the metric names to onewordlowercase so I don't have to add duplicate entries

    std::unordered_map<string_view, float> metrics = {
        {"pixel_space", 8.0f}, // Default pixel space
        {"screen_width", static_cast<float>(NDS_SCREEN_WIDTH)},
        {"screen_height", static_cast<float>(NDS_SCREEN_HEIGHT)}
        // TODO: max screen width (for renderer)
        // TODO: max screen height (for renderer)
        // TODO: scale factor

    };
    // TODO: Parse the VFL string and populate the matrices
}*/