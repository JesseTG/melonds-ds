/**
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

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/mat3x3.hpp>


#include "environment.hpp"

namespace MelonDsDs {
    enum class ScreenType {
        Top,
        Touch,
        ThreeD, // Whichever screen most recently displayed 3D content
    };

    struct Screen {
        glm::ivec2 position;
        glm::vec2 scale {1.0f, 1.0f};
        ScreenType type;
    };

    struct ScreenLayoutSpec {
        std::vector<Screen> screens;
        std::string name;
        retro::ScreenOrientation orientation = retro::ScreenOrientation::Normal;

    };

    /**
     * Parses a TOML-formatted string to extract screen layout specifications.
     *
     * @param toml TOML-formatted string containing the screen layout configuration.
     * @return A vector of ScreenLayoutSpec objects parsed from the TOML,
     * with invalid entries filtered out.
     */
    std::vector<ScreenLayoutSpec> Parse(std::string_view toml) noexcept;
}
