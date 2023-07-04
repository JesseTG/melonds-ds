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

#ifndef MELONDS_DS_SCREENLAYOUT_HPP
#define MELONDS_DS_SCREENLAYOUT_HPP

#include <cstddef>
#include <cstdint>
#include "config.hpp"

namespace melonds {
    /// The native width of the Nintendo DS screens, in pixels
    constexpr int VIDEO_WIDTH = 256;

    /// The native height of the Nintendo DS screens, in pixels
    constexpr int VIDEO_HEIGHT = 192;

    ScreenLayout SwapLayout(ScreenLayout layout) noexcept;

    class ScreenLayoutData {
    public:
        ScreenLayoutData();
        void copy_screen(uint32_t* src, unsigned offset);
        void copy_hybrid_screen(uint32_t* src, ScreenId screen_id);
        [[deprecate("Move to render.cpp")]] void draw_cursor(int32_t x, int32_t y);
        void clean_screenlayout_buffer();

        void Update(melonds::Renderer renderer) noexcept;

        void* Buffer() noexcept { return buffer_ptr; }
        const void* Buffer() const noexcept { return buffer_ptr; }

        unsigned BufferWidth() const noexcept { return buffer_width; }
        unsigned BufferHeight() const noexcept { return buffer_height; }
        unsigned BufferStride() const noexcept { return buffer_stride; }
        float BufferAspectRatio() const noexcept { return float(buffer_width) / float(buffer_height); }

        unsigned ScreenWidth() const noexcept { return screen_width; }
        unsigned ScreenHeight() const noexcept { return screen_height; }

        void SwapScreens(bool swap) noexcept { swapScreens = swap; }
        bool SwapScreens() const noexcept { return swapScreens; }

        ScreenLayout Layout() const noexcept { return layout; }
        void Layout(ScreenLayout _layout) noexcept { this->layout = _layout; }
        ScreenLayout SwappedLayout() const noexcept { return SwapLayout(layout); }
        ScreenLayout EffectiveLayout() const noexcept { return swapScreens ? SwappedLayout() : layout; }

        bool IsHybridLayout() const noexcept { return layout == ScreenLayout::HybridTop || layout == ScreenLayout::HybridBottom; }
        SmallScreenLayout HybridSmallScreenLayout() const noexcept { return hybrid_small_screen; }
        void HybridSmallScreenLayout(SmallScreenLayout _layout) noexcept { hybrid_small_screen = _layout; }

        bool EffectiveTopScreenEnabled() const noexcept { return EffectiveLayout() != ScreenLayout::BottomOnly; }
        bool EffectiveBottomScreenEnabled() const noexcept { return EffectiveLayout() != ScreenLayout::TopOnly; }

        unsigned ScreenGap() const noexcept { return screen_gap_unscaled; }
        void ScreenGap(unsigned _screen_gap) noexcept { screen_gap_unscaled = _screen_gap; }
        unsigned ScaledScreenGap() const noexcept { return screen_gap_unscaled * scale; }

        unsigned HybridRatio() const noexcept { return hybrid_ratio; }
        void HybridRatio(unsigned _hybrid_ratio) noexcept { hybrid_ratio = _hybrid_ratio; }

        unsigned TopScreenOffset() const noexcept { return top_screen_offset; }
        unsigned BottomScreenOffset() const noexcept { return bottom_screen_offset; }

        unsigned TouchOffsetX() const noexcept { return touch_offset_x; }
        unsigned TouchOffsetY() const noexcept { return touch_offset_y; }
    private:
        bool swapScreens;
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

        SmallScreenLayout hybrid_small_screen;
        unsigned hybrid_ratio;

        unsigned buffer_width;
        unsigned buffer_height;
        unsigned buffer_stride;
        size_t buffer_len;
        uint16_t *buffer_ptr;
        ScreenLayout layout;
    };

    extern ScreenLayoutData screen_layout_data;
    extern bool ScreenSwap;
}
#endif //MELONDS_DS_SCREENLAYOUT_HPP
