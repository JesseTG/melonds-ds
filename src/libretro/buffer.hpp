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

#ifndef MELONDS_DS_BUFFER_HPP
#define MELONDS_DS_BUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <glm/vec2.hpp>

namespace melonds {
    class PixelBuffer {
    public:
        PixelBuffer(glm::uvec2 size) noexcept;
        PixelBuffer(std::nullptr_t) noexcept;
        ~PixelBuffer() noexcept;
        PixelBuffer(const PixelBuffer&) noexcept;
        PixelBuffer(PixelBuffer&&) noexcept;
        PixelBuffer& operator=(const PixelBuffer&) noexcept;
        PixelBuffer& operator=(PixelBuffer&&) noexcept;
        PixelBuffer& operator=(std::nullptr_t) noexcept;

        [[nodiscard]] uint32_t operator[](glm::uvec2 pos) const noexcept {
            return buffer[pos.y * stride + pos.x];
        }

        [[nodiscard]] uint32_t& operator[](glm::uvec2 pos) noexcept {
            return buffer[pos.y * stride + pos.x];
        }

        [[nodiscard]] uint32_t* operator[](unsigned row) noexcept {
            return buffer + row * stride;
        }

        [[nodiscard]] const uint32_t* operator[](unsigned row) const noexcept {
            return buffer + row * stride;
        }

        operator bool() const noexcept { return buffer != nullptr; }

        glm::uvec2 Size() const noexcept { return size; }
        unsigned Width() const noexcept { return size.x; }
        unsigned Height() const noexcept { return size.y; }
        unsigned Stride() const noexcept { return stride; }
        uint32_t *Buffer() noexcept { return buffer; }
        const uint32_t *Buffer() const noexcept { return buffer; }
        void Clear() noexcept;
    private:
        glm::uvec2 size;
        unsigned stride;
        uint32_t *buffer;
    };
}

#endif //MELONDS_DS_BUFFER_HPP
