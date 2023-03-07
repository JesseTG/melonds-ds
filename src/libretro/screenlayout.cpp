//
// Created by Jesse on 3/6/2023.
//

#include "libretro.hpp"
#include "screenlayout.hpp"

namespace melonds {
    static ScreenLayout _current_screen_layout = ScreenLayout::TopBottom;
    static ScreenLayoutData _screen_layout;

    ScreenLayout current_screen_layout() {
        return _current_screen_layout;
    }
}


melonds::ScreenLayoutData::ScreenLayoutData() {
    this->buffer_ptr = nullptr;
    this->hybrid_ratio = 2;
}

PUBLIC_SYMBOL void retro_get_system_av_info(struct retro_system_av_info *info) {
    using melonds::_screen_layout;

    info->timing.fps = 32.0f * 1024.0f * 1024.0f / 560190.0f;
    info->timing.sample_rate = 32.0f * 1024.0f;
    info->geometry.base_width = _screen_layout.buffer_width;
    info->geometry.base_height = _screen_layout.buffer_height;
    info->geometry.max_width = _screen_layout.buffer_width;
    info->geometry.max_height = _screen_layout.buffer_height;
    info->geometry.aspect_ratio = (float) _screen_layout.buffer_width / (float) _screen_layout.buffer_height;
}