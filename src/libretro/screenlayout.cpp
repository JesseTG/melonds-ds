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
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include "libretro.hpp"
#include "screenlayout.hpp"
#include "config.hpp"
#include <frontend/qt_sdl/Config.h>
#include <functional>
#include <cstring>

namespace melonds {
    static ScreenLayout _current_screen_layout = ScreenLayout::TopBottom;

    ScreenLayoutData screen_layout_data;

    ScreenLayout current_screen_layout() {
        return _current_screen_layout;
    }
}

melonds::ScreenLayoutData::ScreenLayoutData() {
    this->buffer_ptr = nullptr;
    this->hybrid_ratio = 2;
}

void melonds::ScreenLayoutData::copy_screen(uint32_t *src, unsigned offset) {
    if (direct_copy) {
        memcpy((uint32_t *) buffer_ptr + offset, src, screen_width * screen_height * pixel_size);
    } else {
        unsigned y;
        for (y = 0; y < screen_height; y++) {
            memcpy((uint16_t *) buffer_ptr + offset + (y * screen_width * pixel_size),
                   src + (y * screen_width), screen_width * pixel_size);
        }
    }

}

void melonds::ScreenLayoutData::copy_hybrid_screen(uint32_t *src, ScreenId screen_id) {
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
                       src + (y * screen_width), (screen_width) * pixel_size);
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
                       src + (y * screen_width), (screen_width) * pixel_size);
            }
        }
            break;
    }
}

void melonds::ScreenLayoutData::draw_cursor(int32_t x, int32_t y) {
    auto *base_offset = (uint32_t *) buffer_ptr;

    uint32_t scale = displayed_layout == ScreenLayout::HybridBottom ? hybrid_ratio : 1;

    uint32_t start_y = std::clamp(y - cursor_size(), 0u, screen_height) * scale;
    uint32_t end_y = std::clamp(y + cursor_size(), 0u, screen_height) * scale;

    for (uint32_t y = start_y; y < end_y; y++) {
        uint32_t start_x = std::clamp(x - cursor_size(), 0u, screen_width) * scale;
        uint32_t end_x = std::clamp(x + cursor_size(), 0u, screen_width) * scale;

        for (uint32_t x = start_x; x < end_x; x++) {
            uint32_t *offset = base_offset + ((y + touch_offset_y) * buffer_width) + ((x + touch_offset_x));
            uint32_t pixel = *offset;
            *(uint32_t *) offset = (0xFFFFFF - pixel) | 0xFF000000;
        }
    }
}


void melonds::ScreenLayoutData::clean_screenlayout_buffer() {
    if (buffer_ptr != nullptr) {
        memset(buffer_ptr, 0, buffer_stride * buffer_height);
    }
}

using melonds::ScreenLayout;
using melonds::ScreenLayoutData;

void melonds::update_screenlayout(ScreenLayout layout, ScreenLayoutData *data, bool opengl, bool swap_screens) {
    unsigned pixel_size = 4; // XRGB8888 is hardcoded for now, so it's fine
    data->pixel_size = pixel_size;

    unsigned scale = 1; // ONLY SUPPORTED BY OPENGL RENDERER

    if (opengl) {
        // To avoid some issues the size should be at least 4x the native res
        if (Config::GL_ScaleFactor > 4)
            scale = Config::GL_ScaleFactor;
        else
            scale = 4;
    }

    data->scale = scale;

    unsigned old_size = data->buffer_stride * data->buffer_height;

    data->direct_copy = false;
    data->hybrid = false;

    data->screen_width = melonds::VIDEO_WIDTH * scale;
    data->screen_height = melonds::VIDEO_HEIGHT * scale;
    data->screen_gap = data->screen_gap_unscaled * scale;

    melonds::_current_screen_layout = layout;

    if (swap_screens) {
        switch (melonds::_current_screen_layout) {
            case ScreenLayout::BottomOnly:
                layout = ScreenLayout::TopOnly;
                break;
            case ScreenLayout::TopOnly:
                layout = ScreenLayout::BottomOnly;
                break;
            case ScreenLayout::BottomTop:
                layout = ScreenLayout::TopBottom;
                break;
            case ScreenLayout::TopBottom:
                layout = ScreenLayout::BottomTop;
                break;
            case ScreenLayout::LeftRight:
                layout = ScreenLayout::RightLeft;
                break;
            case ScreenLayout::RightLeft:
                layout = ScreenLayout::LeftRight;
                break;
            case ScreenLayout::HybridTop:
                layout = ScreenLayout::HybridBottom;
                break;
            case ScreenLayout::HybridBottom:
                layout = ScreenLayout::HybridTop;
                break;
        }
    }

    switch (layout) {
        case ScreenLayout::TopBottom:
            data->enable_top_screen = true;
            data->enable_bottom_screen = true;
            data->direct_copy = true;

            data->buffer_width = data->screen_width;
            data->buffer_height = data->screen_height * 2 + data->screen_gap;
            data->buffer_stride = data->screen_width * pixel_size;

            data->touch_offset_x = 0;
            data->touch_offset_y = data->screen_height + data->screen_gap;

            data->top_screen_offset = 0;
            data->bottom_screen_offset = data->buffer_width * (data->screen_height + data->screen_gap);

            break;
        case ScreenLayout::BottomTop:
            data->enable_top_screen = true;
            data->enable_bottom_screen = true;
            data->direct_copy = true;

            data->buffer_width = data->screen_width;
            data->buffer_height = data->screen_height * 2 + data->screen_gap;
            data->buffer_stride = data->screen_width * pixel_size;

            data->touch_offset_x = 0;
            data->touch_offset_y = 0;

            data->top_screen_offset = data->buffer_width * (data->screen_height + data->screen_gap);
            data->bottom_screen_offset = 0;

            break;
        case ScreenLayout::LeftRight:
            data->enable_top_screen = true;
            data->enable_bottom_screen = true;

            data->buffer_width = data->screen_width * 2;
            data->buffer_height = data->screen_height;
            data->buffer_stride = data->screen_width * 2 * pixel_size;

            data->touch_offset_x = data->screen_width;
            data->touch_offset_y = 0;

            data->top_screen_offset = 0;
            data->bottom_screen_offset = (data->screen_width * 2);

            break;
        case ScreenLayout::RightLeft:
            data->enable_top_screen = true;
            data->enable_bottom_screen = true;

            data->buffer_width = data->screen_width * 2;
            data->buffer_height = data->screen_height;
            data->buffer_stride = data->screen_width * 2 * pixel_size;

            data->touch_offset_x = 0;
            data->touch_offset_y = 0;

            data->top_screen_offset = (data->screen_width * 2);
            data->bottom_screen_offset = 0;

            break;
        case ScreenLayout::TopOnly:
            data->enable_top_screen = true;
            data->enable_bottom_screen = false;
            data->direct_copy = true;

            data->buffer_width = data->screen_width;
            data->buffer_height = data->screen_height;
            data->buffer_stride = data->screen_width * pixel_size;

            // should be disabled in top only
            data->touch_offset_x = 0;
            data->touch_offset_y = 0;

            data->top_screen_offset = 0;

            break;
        case ScreenLayout::BottomOnly:
            data->enable_top_screen = false;
            data->enable_bottom_screen = true;
            data->direct_copy = true;

            data->buffer_width = data->screen_width;
            data->buffer_height = data->screen_height;
            data->buffer_stride = data->screen_width * pixel_size;

            data->touch_offset_x = 0;
            data->touch_offset_y = 0;

            data->bottom_screen_offset = 0;

            break;
        case ScreenLayout::HybridTop:
        case ScreenLayout::HybridBottom:
            data->enable_top_screen = true;
            data->enable_bottom_screen = true;

            data->hybrid = true;

            data->buffer_width =
                    (data->screen_width * data->hybrid_ratio) + data->screen_width + (data->hybrid_ratio * 2);
            data->buffer_height = (data->screen_height * data->hybrid_ratio);
            data->buffer_stride = data->buffer_width * pixel_size;

            if (layout == ScreenLayout::HybridTop) {
                data->touch_offset_x = (data->screen_width * data->hybrid_ratio) + (data->hybrid_ratio / 2);
                data->touch_offset_y = (data->screen_height * (data->hybrid_ratio - 1));
            } else {
                data->touch_offset_x = 0;
                data->touch_offset_y = 0;
            }

            break;
    }

    data->displayed_layout = layout;

    if (opengl && data->buffer_ptr != nullptr) {
        // not needed anymore :)
        free(data->buffer_ptr);
        data->buffer_ptr = nullptr;
    } else {
        unsigned new_size = data->buffer_stride * data->buffer_height;

        if (old_size != new_size || data->buffer_ptr == nullptr) {
            if (data->buffer_ptr != nullptr) free(data->buffer_ptr);
            data->buffer_ptr = (uint16_t *) malloc(new_size);

            memset(data->buffer_ptr, 0, new_size);
        }
    }
}

PUBLIC_SYMBOL void retro_get_system_av_info(struct retro_system_av_info *info) {
    using melonds::screen_layout_data;

    info->timing.fps = 32.0f * 1024.0f * 1024.0f / 560190.0f;
    info->timing.sample_rate = 32.0f * 1024.0f;
    info->geometry.base_width = screen_layout_data.buffer_width;
    info->geometry.base_height = screen_layout_data.buffer_height;
    info->geometry.max_width = screen_layout_data.buffer_width;
    info->geometry.max_height = screen_layout_data.buffer_height;
    info->geometry.aspect_ratio = (float) screen_layout_data.buffer_width / (float) screen_layout_data.buffer_height;
}