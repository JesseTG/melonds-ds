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

#ifndef MELONDS_DS_MATH_HPP
#define MELONDS_DS_MATH_HPP


#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace melonds::math {

    /// @brief Create a 3x3 transformation matrix from a translation and a scale.
    template<typename T>
    constexpr GLM_FUNC_QUALIFIER glm::tmat3x3<T> ts(glm::tvec2<T> translation, glm::tvec2<T> scale) noexcept
    {
        return glm::tmat3x3<T> {
            glm::tvec3<T>(scale.x, 0, 0), // column 0
            glm::tvec3<T>(0, scale.y, 0), // column 1,
            glm::tvec3<T>(translation, 1),
        };
    }
}

#endif //MELONDS_DS_MATH_HPP
