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

MelonDsDs::PixelBuffer::PixelBuffer(unsigned width, unsigned height) noexcept :
    PixelBuffer(uvec2(width, height)) {
}

MelonDsDs::PixelBuffer::PixelBuffer(uvec2 size) noexcept :
    size(size),
    stride(size.x * sizeof(uint32_t)),
    buffer(size.x * size.y, 0) {
}

void MelonDsDs::PixelBuffer::SetSize(uvec2 newSize) noexcept {
    ZoneScopedN(TracyFunction);
    if (newSize == size)
        return;

    size = newSize;
    stride = size.x * sizeof(uint32_t);
    buffer.resize(size.x * size.y);
}

void MelonDsDs::PixelBuffer::Clear() noexcept {
    memset(buffer.data(), 0, buffer.size());
}

void MelonDsDs::PixelBuffer::CopyDirect(const uint32_t* source, uvec2 destination) noexcept {
    ZoneScopedN(TracyFunction);
    memcpy(&this->operator[](destination), source, NDS_SCREEN_AREA<size_t> * PIXEL_SIZE);
}

void MelonDsDs::PixelBuffer::CopyRows(const uint32_t* source, uvec2 destination, uvec2 destinationSize) noexcept {
    ZoneScopedN(TracyFunction);
    for (unsigned y = 0; y < destinationSize.y; y++) {
        // For each row of the rendered screen...
        memcpy(
            &this->operator[](uvec2(destination.x, destination.y + y)),
            source + (y * destinationSize.x),
            destinationSize.x * PIXEL_SIZE
        );
    }
}