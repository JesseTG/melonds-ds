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

#include "libretro.hpp"
#include "screenlayout.hpp"
#include "config.hpp"
#include <functional>
#include <cstring>

melonds::ScreenLayoutData::ScreenLayoutData() {
    this->buffer_ptr = nullptr;
    this->hybrid_ratio = 2;
    this->_dirty = true; // Uninitialized
    this->_numberOfLayouts = 1;
}

melonds::ScreenLayoutData::~ScreenLayoutData() {
    free(buffer_ptr);
}

void melonds::ScreenLayoutData::CopyScreen(const uint32_t* src, unsigned offset) noexcept {
    if (direct_copy) {
        memcpy((uint32_t *) buffer_ptr + offset, src, screen_width * screen_height * PIXEL_SIZE);
    } else {
        unsigned y;
        for (y = 0; y < screen_height; y++) {
            memcpy((uint16_t *) buffer_ptr + offset + (y * screen_width * PIXEL_SIZE),
                   src + (y * screen_width), screen_width * PIXEL_SIZE);
        }
    }

}

void melonds::ScreenLayoutData::CopyHybridScreen(const uint32_t* src, ScreenId screen_id) noexcept {
    switch (screen_id) {
        case ScreenId::Primary: {
            unsigned buffer_y, buffer_x;
            unsigned x, y, pixel;
            uint32_t pixel_data;
            unsigned buffer_height = screen_height * hybrid_ratio;
            unsigned buffer_width = screen_width * hybrid_ratio;

            for (buffer_y = 0; buffer_y < buffer_height; buffer_y++) {
                y = buffer_y / hybrid_ratio;
                for (buffer_x = 0; buffer_x < buffer_width; buffer_x++) {
                    x = buffer_x / hybrid_ratio;

                    pixel_data = *(uint32_t *) (src + (y * screen_width) + x);

                    for (pixel = 0; pixel < hybrid_ratio; pixel++) {
                        *(uint32_t *) (buffer_ptr + (buffer_y * buffer_stride / 2) + pixel * 2 +
                                       (buffer_x * 2)) = pixel_data;
                    }
                }
            }
        }
            break;
        case ScreenId::Top: {
            unsigned y;
            for (y = 0; y < screen_height; y++) {
                memcpy((uint16_t *) buffer_ptr
                       // X
                       + ((screen_width * hybrid_ratio * 2) +
                          (hybrid_ratio % 2 == 0 ? hybrid_ratio : ((hybrid_ratio / 2) * 4)))
                       // Y
                       + (y * buffer_stride / 2),
                       src + (y * screen_width), (screen_width) * PIXEL_SIZE);
            }
        }
            break;
        case ScreenId::Bottom: {
            unsigned y;
            for (y = 0; y < screen_height; y++) {
                memcpy((uint16_t *) buffer_ptr
                       // X
                       + ((screen_width * hybrid_ratio * 2) +
                          (hybrid_ratio % 2 == 0 ? hybrid_ratio : ((hybrid_ratio / 2) * 4)))
                       // Y
                       + ((y + (screen_height * (hybrid_ratio - 1))) * buffer_stride / 2),
                       src + (y * screen_width), (screen_width) * PIXEL_SIZE);
            }
        }
            break;
    }
}

void melonds::ScreenLayoutData::draw_cursor(int32_t x, int32_t y) {
    auto *base_offset = (uint32_t *) buffer_ptr;

    uint32_t scale = Layout() == ScreenLayout::HybridBottom ? hybrid_ratio : 1;
    float cursorSize = melonds::config::video::CursorSize();
    uint32_t start_y = std::clamp<float>(y - cursorSize, 0, screen_height) * scale;
    uint32_t end_y = std::clamp<float>(y + cursorSize, 0, screen_height) * scale;

    for (uint32_t y = start_y; y < end_y; y++) {
        uint32_t start_x = std::clamp<float>(x - cursorSize, 0, screen_width) * scale;
        uint32_t end_x = std::clamp<float>(x + cursorSize, 0, screen_width) * scale;

        for (uint32_t x = start_x; x < end_x; x++) {
            uint32_t *offset = base_offset + ((y + touch_offset_y) * buffer_width) + ((x + touch_offset_x));
            uint32_t pixel = *offset;
            *(uint32_t *) offset = (0xFFFFFF - pixel) | 0xFF000000;
        }
    }
}


void melonds::ScreenLayoutData::Clear() {
    if (buffer_ptr != nullptr) {
        memset(buffer_ptr, 0, buffer_stride * buffer_height);
    }
}

using melonds::ScreenLayoutData;

void melonds::ScreenLayoutData::Update(melonds::Renderer renderer) noexcept {
    if (renderer == Renderer::OpenGl) {
        // To avoid some issues the size should be at least 4x the native res
        if (config::video::ScaleFactor() > 4)
            scale = config::video::ScaleFactor();
        else
            scale = 4;
    } else {
        this->scale = 1;
    }

    unsigned old_size = this->buffer_stride * this->buffer_height;

    this->direct_copy = false;

    this->screen_width = melonds::NDS_SCREEN_WIDTH * scale;
    this->screen_height = melonds::NDS_SCREEN_HEIGHT * scale;
    unsigned scaledScreenGap = ScaledScreenGap();

    switch (Layout()) {
        case ScreenLayout::TopBottom:
            this->direct_copy = true;

            this->buffer_width = this->screen_width;
            this->buffer_height = this->screen_height * 2 + scaledScreenGap;
            this->buffer_stride = this->screen_width * PIXEL_SIZE;

            this->touch_offset_x = 0;
            this->touch_offset_y = this->screen_height + scaledScreenGap;

            this->top_screen_offset = 0;
            this->bottom_screen_offset = this->buffer_width * (this->screen_height + scaledScreenGap);

            break;
        case ScreenLayout::BottomTop:
            this->direct_copy = true;

            this->buffer_width = this->screen_width;
            this->buffer_height = this->screen_height * 2 + scaledScreenGap;
            this->buffer_stride = this->screen_width * PIXEL_SIZE;

            this->touch_offset_x = 0;
            this->touch_offset_y = 0;

            this->top_screen_offset = this->buffer_width * (this->screen_height + scaledScreenGap);
            this->bottom_screen_offset = 0;

            break;
        case ScreenLayout::LeftRight:
            this->buffer_width = this->screen_width * 2;
            this->buffer_height = this->screen_height;
            this->buffer_stride = this->screen_width * 2 * PIXEL_SIZE;

            this->touch_offset_x = this->screen_width;
            this->touch_offset_y = 0;

            this->top_screen_offset = 0;
            this->bottom_screen_offset = (this->screen_width * 2);

            break;
        case ScreenLayout::RightLeft:

            this->buffer_width = this->screen_width * 2;
            this->buffer_height = this->screen_height;
            this->buffer_stride = this->screen_width * 2 * PIXEL_SIZE;

            this->touch_offset_x = 0;
            this->touch_offset_y = 0;

            this->top_screen_offset = (this->screen_width * 2);
            this->bottom_screen_offset = 0;

            break;
        case ScreenLayout::TopOnly:
            this->direct_copy = true;

            this->buffer_width = this->screen_width;
            this->buffer_height = this->screen_height;
            this->buffer_stride = this->screen_width * PIXEL_SIZE;

            // should be disabled in top only
            this->touch_offset_x = 0;
            this->touch_offset_y = 0;

            this->top_screen_offset = 0;

            break;
        case ScreenLayout::BottomOnly:
            this->direct_copy = true;

            this->buffer_width = this->screen_width;
            this->buffer_height = this->screen_height;
            this->buffer_stride = this->screen_width * PIXEL_SIZE;

            this->touch_offset_x = 0;
            this->touch_offset_y = 0;

            this->bottom_screen_offset = 0;

            break;
        case ScreenLayout::HybridTop:
        case ScreenLayout::HybridBottom:

            this->buffer_width =
                (this->screen_width * this->hybrid_ratio) + this->screen_width + (this->hybrid_ratio * 2);
            this->buffer_height = (this->screen_height * this->hybrid_ratio);
            this->buffer_stride = this->buffer_width * PIXEL_SIZE;

            if (Layout() == ScreenLayout::HybridTop) {
                this->touch_offset_x = (this->screen_width * this->hybrid_ratio) + (this->hybrid_ratio / 2);
                this->touch_offset_y = (this->screen_height * (this->hybrid_ratio - 1));
            } else {
                this->touch_offset_x = 0;
                this->touch_offset_y = 0;
            }

            break;
        // TODO: Implement rotated-left, rotated-right, and upside-down layouts
    }

    if (renderer == Renderer::OpenGl && this->buffer_ptr != nullptr) {
        // not needed anymore :)
        free(this->buffer_ptr);
        this->buffer_ptr = nullptr;
    } else {
        unsigned new_size = this->buffer_stride * this->buffer_height;

        if (old_size != new_size || this->buffer_ptr == nullptr) {
            if (this->buffer_ptr != nullptr) free(this->buffer_ptr);
            this->buffer_ptr = (uint16_t *) malloc(new_size);

            memset(this->buffer_ptr, 0, new_size);
        }
    }

    _dirty = false;
}

retro_game_geometry melonds::ScreenLayoutData::Geometry(melonds::Renderer renderer) const noexcept {
    return {
        .base_width = BufferWidth(),
        .base_height = BufferHeight(),
        .max_width = MaxSoftwareRenderedWidth(), // TODO: Compute for OpenGL
        .max_height = MaxSoftwareRenderedHeight(), // TODO: Compute for OpenGL
        .aspect_ratio = BufferAspectRatio(),
    };
}