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
#include "screenlayout.hpp"
#include "tracy.hpp"

#include <cstring>

using glm::uvec2;

MelonDsDs::PixelBuffer::PixelBuffer(uvec2 size) noexcept :
    size(size),
    stride(size.x * sizeof(uint32_t)),
    buffer(new uint32_t[size.x * size.y]) {
    memset(buffer.get(), 0, size.x * size.y * sizeof(uint32_t));
}

MelonDsDs::PixelBuffer::PixelBuffer(const PixelBuffer& other) noexcept :
    size(other.size),
    stride(other.stride),
    buffer(new uint32_t[other.size.x * other.size.y]) {
    memcpy(buffer.get(), other.buffer.get(), size.x * size.y * sizeof(uint32_t));
}

MelonDsDs::PixelBuffer::PixelBuffer(PixelBuffer&& other) noexcept :
    size(other.size),
    stride(other.stride),
    buffer(std::move(other.buffer)) {
    other.buffer = nullptr;
}

MelonDsDs::PixelBuffer& MelonDsDs::PixelBuffer::operator=(const PixelBuffer& other) noexcept {
    if (this != &other) {
        size = other.size;
        stride = other.stride;
        buffer = std::make_unique<uint32_t[]>(size.x * size.y);
        memcpy(buffer.get(), other.buffer.get(), size.x * size.y * sizeof(uint32_t));
    }
    return *this;
}

MelonDsDs::PixelBuffer& MelonDsDs::PixelBuffer::operator=(PixelBuffer&& other) noexcept {
    if (this != &other) {
        size = other.size;
        stride = other.stride;
        buffer = std::move(other.buffer);
    }
    return *this;
}

void MelonDsDs::PixelBuffer::Clear() noexcept {
    if (buffer) {
        memset(buffer.get(), 0, size.x * size.y * sizeof(uint32_t));
    }
}

void MelonDsDs::PixelBuffer::CopyDirect(const uint32_t* source, uvec2 destination) noexcept {
    ZoneScopedN("MelonDsDs::PixelBuffer::CopyDirect");
    memcpy(&this->operator[](destination), source, NDS_SCREEN_AREA<size_t> * PIXEL_SIZE);
}

void MelonDsDs::PixelBuffer::CopyRows(const uint32_t* source, uvec2 destination, uvec2 destinationSize) noexcept {
    ZoneScopedN("MelonDsDs::PixelBuffer::CopyRows");
    for (unsigned y = 0; y < destinationSize.y; y++) {
        // For each row of the rendered screen...
        memcpy(
            &this->operator[](uvec2(destination.x, destination.y + y)),
            source + (y * destinationSize.x),
            destinationSize.x * PIXEL_SIZE
        );
    }
}