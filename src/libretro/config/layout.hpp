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
#include <optional>
#include <variant>

#include <glm/vec2.hpp>
#include <glm/mat3x3.hpp>
#include <toml11/fwd/error_info_fwd.hpp>
#include <toml11/ordered_map.hpp>

#include "config/types.hpp"
#include "environment.hpp"

namespace MelonDsDs::Layout {
    enum class ScreenType {
        Top,
        Touch,
        ThreeD, // Whichever screen most recently displayed 3D content
    };

    using ParsedExpression = std::variant<int64_t, double, std::string>;

    struct Screen {
        glm::ivec2 position;
        glm::vec2 scale {1.0f, 1.0f};
        ScreenType type;
        bool visible = true;
        std::optional<ScreenFilter> filter = std::nullopt;
    };

    struct ParsedVector {
        ParsedExpression x;
        ParsedExpression y;
    };

    struct ParsedScreen {
        ParsedVector position;
        ParsedVector scale { 1.0, 1.0 };
        ScreenType type;
        std::optional<ParsedExpression> visible = std::nullopt; // If not specified, defaults to true
        std::optional<ScreenFilter> filter = std::nullopt;
    };

    struct ParsedLayout {
        std::vector<ParsedScreen> screens;
        std::optional<std::string> name;
        std::optional<retro::ScreenOrientation> orientation = retro::ScreenOrientation::Normal;
    };

    struct ParsedLayoutConfig {
        // The constructor won't throw any exceptions because
        // I want to be able to exclude invalid layouts
        // without affecting valid ones.
        // Errors are exposed because I _do_ want to log them.
        explicit ParsedLayoutConfig(const std::string& toml) noexcept;
        explicit ParsedLayoutConfig(std::string&& toml) noexcept;

        toml::ordered_map<std::string, ParsedLayout> layouts;
        std::vector<toml::error_info> errors;

        /// Return true if at least one layout was parsed successfully
        [[nodiscard]] explicit operator bool() const noexcept;
    };
}
