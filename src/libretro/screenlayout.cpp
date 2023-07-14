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

#include "screenlayout.hpp"

#include <cstring>
#include <functional>

#include <glm/gtx/matrix_transform_2d.hpp>

#include "config.hpp"

using glm::ivec2;
using glm::ivec3;
using glm::uvec2;
using glm::vec2;
using glm::mat3;

melonds::ScreenLayoutData::ScreenLayoutData() :
    _dirty(true), // Uninitialized
    touchMatrix(1), // Identity matrix
    hybridRatio(2),
    _numberOfLayouts(1),
    buffer(nullptr) {
}

melonds::ScreenLayoutData::~ScreenLayoutData() {
    free(buffer);
}

void melonds::ScreenLayoutData::CopyScreen(const uint32_t* src, unsigned offset) noexcept {
    // Only used for software rendering
    if (LayoutSupportsDirectCopy(Layout())) {
        memcpy((uint32_t *) buffer + offset, src, NDS_SCREEN_AREA<size_t> * PIXEL_SIZE);
    } else {
        for (unsigned y = 0; y < NDS_SCREEN_HEIGHT; y++) {
            memcpy(
                (uint16_t *) buffer + offset + (y * NDS_SCREEN_WIDTH * PIXEL_SIZE),
                src + (y * NDS_SCREEN_WIDTH),
                NDS_SCREEN_WIDTH * PIXEL_SIZE
            );
        }
    }

}

void melonds::ScreenLayoutData::CopyHybridScreen(const uint32_t* src, HybridScreenId screen_id) noexcept {
    // Only used for software rendering
    switch (screen_id) {
        case HybridScreenId::Primary: {
            // TODO: Use one of the libretro scalers instead of this loop
            unsigned buffer_height = NDS_SCREEN_HEIGHT * hybridRatio;
            unsigned buffer_width = NDS_SCREEN_WIDTH * hybridRatio;

            for (unsigned buffer_y = 0; buffer_y < buffer_height; buffer_y++) {
                unsigned y = buffer_y / hybridRatio;
                for (unsigned buffer_x = 0; buffer_x < buffer_width; buffer_x++) {
                    unsigned x = buffer_x / hybridRatio;

                    uint32_t pixel_data = *(uint32_t *) (src + (y * NDS_SCREEN_WIDTH) + x);

                    for (unsigned pixel = 0; pixel < hybridRatio; pixel++) {
                        *(uint32_t *) (buffer + (buffer_y * bufferStride / 2) + pixel * 2 +
                                       (buffer_x * 2)) = pixel_data;
                    }
                }
            }

            break;
        }
        case HybridScreenId::Top: {
            for (unsigned y = 0; y < NDS_SCREEN_HEIGHT; y++) {
                memcpy((uint16_t *) buffer
                       // X
                       + ((NDS_SCREEN_WIDTH * hybridRatio * 2) +
                          (hybridRatio % 2 == 0 ? hybridRatio : ((hybridRatio / 2) * 4)))
                       // Y
                       + (y * bufferStride / 2),
                       src + (y * NDS_SCREEN_WIDTH),
                       NDS_SCREEN_WIDTH * PIXEL_SIZE
               );
            }

            break;
        }
        case HybridScreenId::Bottom: {
            for (unsigned y = 0; y < NDS_SCREEN_HEIGHT; y++) {
                memcpy((uint16_t *) buffer
                       // X
                       + ((NDS_SCREEN_WIDTH * hybridRatio * 2) +
                          (hybridRatio % 2 == 0 ? hybridRatio : ((hybridRatio / 2) * 4)))
                       // Y
                       + ((y + (NDS_SCREEN_HEIGHT * (hybridRatio - 1))) * bufferStride / 2),
                       src + (y * NDS_SCREEN_WIDTH),
                       NDS_SCREEN_WIDTH * PIXEL_SIZE);
            }

            break;
        }
    }
}

void melonds::ScreenLayoutData::DrawCursor(ivec2 touch) noexcept {
    // Only used for software rendering
    if (!buffer)
        return;

    auto *base_offset = (uint32_t *) buffer;

    uint32_t scale = Layout() == ScreenLayout::HybridBottom ? hybridRatio : 1;
    float cursorSize = melonds::config::video::CursorSize();
    uint32_t start_y = std::clamp<float>(touch.y - cursorSize, 0, NDS_SCREEN_HEIGHT) * scale;
    uint32_t end_y = std::clamp<float>(touch.y + cursorSize, 0, NDS_SCREEN_HEIGHT) * scale;

    for (uint32_t y = start_y; y < end_y; y++) {
        uint32_t start_x = std::clamp<float>(touch.x - cursorSize, 0, NDS_SCREEN_WIDTH) * scale;
        uint32_t end_x = std::clamp<float>(touch.x + cursorSize, 0, NDS_SCREEN_WIDTH) * scale;

        for (uint32_t x = start_x; x < end_x; x++) {
            uint32_t *offset = base_offset + ((y + touchOffset.y) * bufferWidth) + ((x + touchOffset.x));
            uint32_t pixel = *offset;
            *(uint32_t *) offset = (0xFFFFFF - pixel) | 0xFF000000;
        }
    }
}


void melonds::ScreenLayoutData::Clear() {
    if (buffer != nullptr) {
        memset(buffer, 0, bufferStride * bufferHeight);
    }
}

using melonds::ScreenLayoutData;

void melonds::ScreenLayoutData::Update(melonds::Renderer renderer) noexcept {
    size_t oldBufferSize = bufferStride * bufferHeight;

    // TODO: Apply the scale to the buffer dimensions later, not here
    ScreenLayout layout = Layout();
    switch (layout) {
        case ScreenLayout::TurnLeft:
        case ScreenLayout::TurnRight:
        case ScreenLayout::UpsideDown:
        case ScreenLayout::TopBottom:
            bufferWidth = NDS_SCREEN_WIDTH * scale;
            bufferHeight = (NDS_SCREEN_HEIGHT * 2 + screenGap) * scale;
            bufferStride = NDS_SCREEN_WIDTH * PIXEL_SIZE * scale;
            touchOffset = uvec2(0, NDS_SCREEN_HEIGHT + screenGap);// * scale;
            topScreenBufferOffset = 0;
            bottomScreenBufferOffset = bufferWidth * (NDS_SCREEN_HEIGHT + screenGap) * scale;
            // We rely on the libretro frontend to rotate the screen for us,
            // so we don't have to treat the buffer dimensions specially here

            break;
        case ScreenLayout::BottomTop:
            bufferWidth = NDS_SCREEN_WIDTH * scale;
            bufferHeight = (NDS_SCREEN_HEIGHT * 2 + screenGap) * scale;
            bufferStride = NDS_SCREEN_WIDTH * PIXEL_SIZE * scale;
            touchOffset = uvec2(0, 0);
            topScreenBufferOffset = bufferWidth * (NDS_SCREEN_HEIGHT + screenGap) * scale;
            bottomScreenBufferOffset = 0;

            break;
        case ScreenLayout::LeftRight:
            bufferWidth = NDS_SCREEN_WIDTH * 2 * scale;
            bufferHeight = NDS_SCREEN_HEIGHT * scale;
            bufferStride = NDS_SCREEN_WIDTH * 2 * PIXEL_SIZE * scale;
            touchOffset = uvec2(NDS_SCREEN_WIDTH, 0);// * scale;
            topScreenBufferOffset = 0;
            bottomScreenBufferOffset = NDS_SCREEN_WIDTH * 2 * scale;

            break;
        case ScreenLayout::RightLeft:
            bufferWidth = NDS_SCREEN_WIDTH * 2 * scale;
            bufferHeight = NDS_SCREEN_HEIGHT * scale;
            bufferStride = NDS_SCREEN_WIDTH * 2 * PIXEL_SIZE * scale;
            touchOffset = uvec2(0, 0);
            topScreenBufferOffset = NDS_SCREEN_WIDTH * 2 * scale;
            bottomScreenBufferOffset = 0;

            break;
        case ScreenLayout::TopOnly:
            bufferWidth = NDS_SCREEN_WIDTH * scale;
            bufferHeight = NDS_SCREEN_HEIGHT * scale;
            bufferStride = NDS_SCREEN_WIDTH * PIXEL_SIZE * scale;
            touchOffset = uvec2(0, 0);
            topScreenBufferOffset = 0;

            break;
        case ScreenLayout::BottomOnly:
            bufferWidth = NDS_SCREEN_WIDTH * scale;
            bufferHeight = NDS_SCREEN_HEIGHT * scale;
            bufferStride = NDS_SCREEN_WIDTH * PIXEL_SIZE * scale;
            touchOffset = uvec2(0, 0);
            bottomScreenBufferOffset = 0;

            break;
        case ScreenLayout::HybridTop:
        case ScreenLayout::HybridBottom:
            bufferWidth = (NDS_SCREEN_WIDTH * hybridRatio * scale) + NDS_SCREEN_WIDTH * scale + (hybridRatio * 2);
            bufferHeight = NDS_SCREEN_HEIGHT * hybridRatio * scale;
            bufferStride = bufferWidth * PIXEL_SIZE;

            if (Layout() == ScreenLayout::HybridTop) {
                touchOffset = uvec2((NDS_SCREEN_WIDTH * hybridRatio) + (hybridRatio / 2), (NDS_SCREEN_HEIGHT * (hybridRatio - 1))); // * scale;
            } else {
                touchOffset = uvec2(0, 0);
            }
            break;
    }

    float rotation = LayoutAngle(layout);
    vec2 scaleVector = {
        (bufferWidth * static_cast<float>(scale)) / std::numeric_limits<uint16_t>::max(),
        (bufferHeight * static_cast<float>(scale)) / std::numeric_limits<uint16_t>::max()
    };
    ivec2 translation = ivec2(touchOffset) - (ivec2(bufferWidth, 0) / 2);

    // This transformation order is important!
    touchMatrix = rotate(mat3(1), rotation) * glm::scale(mat3(1), scaleVector) * glm::translate(mat3(1), vec2(translation));
    if (!retro::set_screen_rotation(LayoutOrientation())) {
        // TODO: Handle this error
    }

    if (renderer == Renderer::OpenGl && this->buffer != nullptr) {
        // not needed anymore :)
        free(this->buffer);
        this->buffer = nullptr;
    } else {
        unsigned new_size = this->bufferStride * this->bufferHeight;

        if (oldBufferSize != new_size || this->buffer == nullptr) {
            if (this->buffer != nullptr) free(this->buffer);
            this->buffer = (uint16_t *) malloc(new_size);

            memset(this->buffer, 0, new_size);
        }
    }

    _dirty = false;
}



retro_game_geometry melonds::ScreenLayoutData::Geometry(melonds::Renderer renderer) const noexcept {
    retro_game_geometry geometry {
        .base_width = BufferWidth(),
        .base_height = BufferHeight(),
        .max_width = MaxSoftwareRenderedWidth(),
        .max_height = MaxSoftwareRenderedHeight(),
        .aspect_ratio = BufferAspectRatio(),
    };

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (renderer == Renderer::OpenGl) {
        geometry.max_width = MaxOpenGlRenderedWidth();
        geometry.max_height = MaxOpenGlRenderedHeight();
    }
#endif

    return geometry;
}