//
// Created by Jesse on 3/6/2023.
//

#ifndef MELONDS_DS_SCREENLAYOUT_HPP
#define MELONDS_DS_SCREENLAYOUT_HPP

#include <cstddef>
#include <cstdint>

namespace melonds {
    constexpr int VIDEO_WIDTH = 256;
    constexpr int VIDEO_HEIGHT = 192;

    enum class SmallScreenLayout {
        SmallScreenTop = 0,
        SmallScreenBottom = 1,
        SmallScreenDuplicate = 2
    };

    enum class ScreenId {
        Primary = 0,
        Top = 1,
        Bottom = 2,
    };

    enum class ScreenLayout {
        TopBottom = 0,
        BottomTop = 1,
        LeftRight = 2,
        RightLeft = 3,
        TopOnly = 4,
        BottomOnly = 5,
        HybridTop = 6,
        HybridBottom = 7,
    };

    struct ScreenLayoutData {
        ScreenLayoutData();
        void copy_screen(uint32_t* src, unsigned offset);
        void copy_hybrid_screen(uint32_t* src, ScreenId screen_id);
        void draw_cursor(int32_t x, int32_t y);
        void clean_screenlayout_buffer();

        bool enable_top_screen;
        bool enable_bottom_screen;
        bool direct_copy;

        unsigned pixel_size;
        unsigned scale;

        unsigned screen_width;
        unsigned screen_height;
        unsigned top_screen_offset;
        unsigned bottom_screen_offset;

        unsigned touch_offset_x;
        unsigned touch_offset_y;

        unsigned screen_gap_unscaled;
        unsigned screen_gap;

        bool hybrid;
        SmallScreenLayout hybrid_small_screen;
        unsigned hybrid_ratio;

        unsigned buffer_width;
        unsigned buffer_height;
        unsigned buffer_stride;
        size_t buffer_len;
        uint16_t *buffer_ptr;
        ScreenLayout displayed_layout;
    };

    ScreenLayout current_screen_layout();

    void update_screenlayout(ScreenLayout layout, ScreenLayoutData *data, bool opengl, bool swap_screens);

    extern ScreenLayoutData screen_layout_data;
}
#endif //MELONDS_DS_SCREENLAYOUT_HPP
