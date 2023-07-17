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

#include "buffer.hpp"

#include <cstring>

melonds::PixelBuffer::PixelBuffer(glm::uvec2 size) noexcept :
    size(size),
    stride(size.x * sizeof(uint32_t)),
    buffer(new uint32_t[size.x * size.y]) {
    memset(buffer, 0, size.x * size.y * sizeof(uint32_t));
}

melonds::PixelBuffer::PixelBuffer(std::nullptr_t) noexcept :
    size(0, 0),
    stride(0),
    buffer(nullptr) {}

melonds::PixelBuffer::~PixelBuffer() noexcept {
    delete[] buffer;
}

melonds::PixelBuffer::PixelBuffer(const PixelBuffer& other) noexcept :
    size(other.size),
    stride(other.stride),
    buffer(new uint32_t[other.size.x * other.size.y]) {
    memcpy(buffer, other.buffer, size.x * size.y * sizeof(uint32_t));
}

melonds::PixelBuffer::PixelBuffer(PixelBuffer&& other) noexcept :
    size(other.size),
    stride(other.stride),
    buffer(other.buffer) {
    other.buffer = nullptr;
}

melonds::PixelBuffer& melonds::PixelBuffer::operator=(const PixelBuffer& other) noexcept {
    if (this != &other) {
        delete[] buffer;
        size = other.size;
        stride = other.stride;
        buffer = new uint32_t[size.x * size.y];
        memcpy(buffer, other.buffer, size.x * size.y * sizeof(uint32_t));
    }
    return *this;
}

melonds::PixelBuffer& melonds::PixelBuffer::operator=(PixelBuffer&& other) noexcept {
    if (this != &other) {
        delete[] buffer;
        size = other.size;
        stride = other.stride;
        buffer = other.buffer;
        other.buffer = nullptr;
    }
    return *this;
}

melonds::PixelBuffer& melonds::PixelBuffer::operator=(std::nullptr_t) noexcept {
    delete[] buffer;
    size = glm::uvec2(0, 0);
    stride = 0;
    buffer = nullptr;
    return *this;
}

void melonds::PixelBuffer::Clear() noexcept {
    if (buffer) {
        memset(buffer, 0, size.x * size.y * sizeof(uint32_t));
    }
}