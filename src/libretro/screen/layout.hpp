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

#pragma once

#include <optional>
#include <string_view>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

namespace MelonDsDs {
    using std::make_optional;
    using std::nullopt;
    using std::optional;
    using std::string_view;
    using std::unordered_map;

    using glm::mat3;

    class ParsedLayout {

    public:
        ParsedLayout(string_view vfl, const unordered_map<string_view, float>& metrics);
    private:
        optional<mat3> topScreenMatrix = mat3(1.0f); // Identity matrix
        optional<mat3> bottomScreenMatrix = mat3(1.0f); // Identity matrix
        optional<mat3> hybridScreenMatrix = nullopt;

    };
}
