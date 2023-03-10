//
// Created by Jesse on 3/7/2023.
//

#ifndef MELONDS_DS_UTILS_HPP
#define MELONDS_DS_UTILS_HPP

#include "screenlayout.hpp"

#if defined(_WIN32)
#define PLATFORM_DIR_SEPERATOR '\\'
#else
#define PLATFORM_DIR_SEPERATOR  '/'
#endif

namespace melonds {
    void copy_screen(ScreenLayoutData *data, uint32_t* src, unsigned offset);
    void copy_hybrid_screen(ScreenLayoutData *data, uint32_t* src, ScreenId screen_id);
    void draw_cursor(ScreenLayoutData *data, int32_t x, int32_t y);
}

#endif //MELONDS_DS_UTILS_HPP
