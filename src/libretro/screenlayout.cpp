//
// Created by Jesse on 3/6/2023.
//

#include "libretro.hpp"
#include "screenlayout.hpp"
#include <frontend/qt_sdl/Config.h>

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

using melonds::ScreenLayout;
using melonds::ScreenLayoutData;
void melonds::update_screenlayout(ScreenLayout layout, ScreenLayoutData *data, bool opengl, bool swap_screens)
{
    unsigned pixel_size = 4; // XRGB8888 is hardcoded for now, so it's fine
    data->pixel_size = pixel_size;

    unsigned scale = 1; // ONLY SUPPORTED BY OPENGL RENDERER

    if(opengl)
    {
        // To avoid some issues the size should be at least 4x the native res
        if(Config::GL_ScaleFactor > 4)
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

    if (swap_screens)
    {
        switch (melonds::_current_screen_layout)
        {
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

    switch (layout)
    {
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

            data->buffer_width = (data->screen_width * data->hybrid_ratio) + data->screen_width + (data->hybrid_ratio * 2);
            data->buffer_height = (data->screen_height * data->hybrid_ratio);
            data->buffer_stride = data->buffer_width * pixel_size;

            if (layout == ScreenLayout::HybridTop)
            {
                data->touch_offset_x = (data->screen_width * data->hybrid_ratio) + (data->hybrid_ratio / 2);
                data->touch_offset_y = (data->screen_height * (data->hybrid_ratio - 1));
            }
            else
            {
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
    }
    else
    {
        unsigned new_size = data->buffer_stride * data->buffer_height;

        if (old_size != new_size || data->buffer_ptr == nullptr)
        {
            if(data->buffer_ptr != nullptr) free(data->buffer_ptr);
            data->buffer_ptr = (uint16_t*)malloc(new_size);

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