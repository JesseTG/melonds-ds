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

using glm::vec2;
using glm::mat3;

melonds::ScreenLayoutData::ScreenLayoutData() :
    _dirty(true), // Uninitialized
    transformMatrix(1), // Identity matrix
    hybridRatio(2),
    _numberOfLayouts(1),
    buffer(nullptr) {
}

melonds::ScreenLayoutData::~ScreenLayoutData() {
    free(buffer);
}

void melonds::ScreenLayoutData::CopyScreen(const uint32_t* src, unsigned offset) noexcept {
    if (LayoutSupportsDirectCopy(Layout())) {
        memcpy((uint32_t *) buffer + offset, src, screenSize.x * screenSize.y * PIXEL_SIZE);
    } else {
        unsigned y;
        for (y = 0; y < screenSize.y; y++) {
            memcpy((uint16_t *) buffer + offset + (y * screenSize.x * PIXEL_SIZE),
                   src + (y * screenSize.x), screenSize.x * PIXEL_SIZE);
        }
    }

}

void melonds::ScreenLayoutData::CopyHybridScreen(const uint32_t* src, HybridScreenId screen_id) noexcept {
    switch (screen_id) {
        case HybridScreenId::Primary: {
            unsigned buffer_y, buffer_x;
            unsigned x, y, pixel;
            uint32_t pixel_data;
            unsigned buffer_height = screenSize.y * hybridRatio;
            unsigned buffer_width = screenSize.x * hybridRatio;

            for (buffer_y = 0; buffer_y < buffer_height; buffer_y++) {
                y = buffer_y / hybridRatio;
                for (buffer_x = 0; buffer_x < buffer_width; buffer_x++) {
                    x = buffer_x / hybridRatio;

                    pixel_data = *(uint32_t *) (src + (y * screenSize.x) + x);

                    for (pixel = 0; pixel < hybridRatio; pixel++) {
                        *(uint32_t *) (buffer + (buffer_y * bufferStride / 2) + pixel * 2 +
                                       (buffer_x * 2)) = pixel_data;
                    }
                }
            }
        }
            break;
        case HybridScreenId::Top: {
            unsigned y;
            for (y = 0; y < screenSize.y; y++) {
                memcpy((uint16_t *) buffer
                       // X
                       + ((screenSize.x * hybridRatio * 2) +
                          (hybridRatio % 2 == 0 ? hybridRatio : ((hybridRatio / 2) * 4)))
                       // Y
                       + (y * bufferStride / 2),
                       src + (y * screenSize.x), (screenSize.x) * PIXEL_SIZE);
            }
        }
            break;
        case HybridScreenId::Bottom: {
            unsigned y;
            for (y = 0; y < screenSize.y; y++) {
                memcpy((uint16_t *) buffer
                       // X
                       + ((screenSize.x * hybridRatio * 2) +
                          (hybridRatio % 2 == 0 ? hybridRatio : ((hybridRatio / 2) * 4)))
                       // Y
                       + ((y + (screenSize.y * (hybridRatio - 1))) * bufferStride / 2),
                       src + (y * screenSize.x), (screenSize.x) * PIXEL_SIZE);
            }
        }
            break;
    }
}

void melonds::ScreenLayoutData::draw_cursor(int32_t x, int32_t y) {
    auto *base_offset = (uint32_t *) buffer;

    uint32_t scale = Layout() == ScreenLayout::HybridBottom ? hybridRatio : 1;
    float cursorSize = melonds::config::video::CursorSize();
    uint32_t start_y = std::clamp<float>(y - cursorSize, 0, screenSize.y) * scale;
    uint32_t end_y = std::clamp<float>(y + cursorSize, 0, screenSize.y) * scale;

    for (uint32_t y = start_y; y < end_y; y++) {
        uint32_t start_x = std::clamp<float>(x - cursorSize, 0, screenSize.x) * scale;
        uint32_t end_x = std::clamp<float>(x + cursorSize, 0, screenSize.x) * scale;

        for (uint32_t x = start_x; x < end_x; x++) {
            uint32_t *offset = base_offset + ((y + touch_offset_y) * bufferWidth) + ((x + touch_offset_x));
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
    unsigned old_size = this->bufferStride * this->bufferHeight;

    this->screenSize.x = melonds::NDS_SCREEN_WIDTH * scale;
    this->screenSize.y = melonds::NDS_SCREEN_HEIGHT * scale;
    unsigned scaledScreenGap = scale * screenGap;

    switch (Layout()) {
        case ScreenLayout::TurnLeft:
        case ScreenLayout::TurnRight:
        case ScreenLayout::UpsideDown:
        case ScreenLayout::TopBottom:
            this->bufferWidth = this->screenSize.x;
            this->bufferHeight = this->screenSize.y * 2 + scaledScreenGap;
            this->bufferStride = this->screenSize.x * PIXEL_SIZE;

            this->touch_offset_x = 0;
            this->touch_offset_y = this->screenSize.y + scaledScreenGap;

            this->topScreenBufferOffset = 0;
            this->bottomScreenBufferOffset = this->bufferWidth * (this->screenSize.y + scaledScreenGap);

            break;
        case ScreenLayout::BottomTop:
            this->bufferWidth = this->screenSize.x;
            this->bufferHeight = this->screenSize.y * 2 + scaledScreenGap;
            this->bufferStride = this->screenSize.x * PIXEL_SIZE;

            this->touch_offset_x = 0;
            this->touch_offset_y = 0;

            this->topScreenBufferOffset = this->bufferWidth * (this->screenSize.y + scaledScreenGap);
            this->bottomScreenBufferOffset = 0;

            break;
        case ScreenLayout::LeftRight:
            this->bufferWidth = this->screenSize.x * 2;
            this->bufferHeight = this->screenSize.y;
            this->bufferStride = this->screenSize.x * 2 * PIXEL_SIZE;

            this->touch_offset_x = this->screenSize.x;
            this->touch_offset_y = 0;

            this->topScreenBufferOffset = 0;
            this->bottomScreenBufferOffset = (this->screenSize.x * 2);

            break;
        case ScreenLayout::RightLeft:

            this->bufferWidth = this->screenSize.x * 2;
            this->bufferHeight = this->screenSize.y;
            this->bufferStride = this->screenSize.x * 2 * PIXEL_SIZE;

            this->touch_offset_x = 0;
            this->touch_offset_y = 0;

            this->topScreenBufferOffset = (this->screenSize.x * 2);
            this->bottomScreenBufferOffset = 0;

            break;
        case ScreenLayout::TopOnly:
            this->bufferWidth = this->screenSize.x;
            this->bufferHeight = this->screenSize.y;
            this->bufferStride = this->screenSize.x * PIXEL_SIZE;

            // should be disabled in top only
            this->touch_offset_x = 0;
            this->touch_offset_y = 0;

            this->topScreenBufferOffset = 0;

            break;
        case ScreenLayout::BottomOnly:
            this->bufferWidth = this->screenSize.x;
            this->bufferHeight = this->screenSize.y;
            this->bufferStride = this->screenSize.x * PIXEL_SIZE;

            this->touch_offset_x = 0;
            this->touch_offset_y = 0;

            this->bottomScreenBufferOffset = 0;

            break;
        case ScreenLayout::HybridTop:
        case ScreenLayout::HybridBottom:

            this->bufferWidth =
                (this->screenSize.x * this->hybridRatio) + this->screenSize.x + (this->hybridRatio * 2);
            this->bufferHeight = (this->screenSize.y * this->hybridRatio);
            this->bufferStride = this->bufferWidth * PIXEL_SIZE;

            if (Layout() == ScreenLayout::HybridTop) {
                this->touch_offset_x = (this->screenSize.x * this->hybridRatio) + (this->hybridRatio / 2);
                this->touch_offset_y = (this->screenSize.y * (this->hybridRatio - 1));
            } else {
                this->touch_offset_x = 0;
                this->touch_offset_y = 0;
            }

            break;
    }

    if (retro::set_screen_rotation(LayoutOrientation())) {
        // TODO: Set input transformation matrix
    } else {
        retro::set_error_message("Failed to rotate screen; effective layout will be Top/Bottom instead.");
    }

    if (renderer == Renderer::OpenGl && this->buffer != nullptr) {
        // not needed anymore :)
        free(this->buffer);
        this->buffer = nullptr;
    } else {
        unsigned new_size = this->bufferStride * this->bufferHeight;

        if (old_size != new_size || this->buffer == nullptr) {
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