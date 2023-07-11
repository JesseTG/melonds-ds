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

#include <libretro.h>

#include <glm/vec2.hpp>

#include "config.hpp"

namespace melonds {
    /// The native width of a single Nintendo DS screen, in pixels
    constexpr int NDS_SCREEN_WIDTH = 256;

    /// The native height of a single Nintendo DS screen, in pixels
    constexpr int NDS_SCREEN_HEIGHT = 192;

    // We require a pixel format of RETRO_PIXEL_FORMAT_XRGB8888, so we can assume 4 bytes here
    constexpr int PIXEL_SIZE = 4;

    class ScreenLayoutData {
    public:
        ScreenLayoutData();
        ~ScreenLayoutData();
        void CopyScreen(const uint32_t* src, unsigned offset) noexcept;
        void CopyHybridScreen(const uint32_t* src, ScreenId screen_id) noexcept;
        [[deprecated("Move to render.cpp")]] void draw_cursor(int32_t x, int32_t y);
        void Clear();

        void Update(Renderer renderer) noexcept;

        bool Dirty() const noexcept { return _dirty; }

        void* Buffer() noexcept { return buffer_ptr; }
        const void* Buffer() const noexcept { return buffer_ptr; }

        unsigned BufferWidth() const noexcept { return buffer_width; }
        unsigned BufferHeight() const noexcept { return buffer_height; }
        unsigned BufferStride() const noexcept { return buffer_stride; }
        float BufferAspectRatio() const noexcept { return float(buffer_width) / float(buffer_height); }

        unsigned ScreenWidth() const noexcept { return screen_size.x; }
        unsigned ScreenHeight() const noexcept { return screen_size.y; }
        glm::uvec2 ScreenSize() const noexcept { return screen_size; }

        unsigned LayoutIndex() const noexcept { return _layoutIndex; }
        unsigned NumberOfLayouts() const noexcept { return _numberOfLayouts; }
        ScreenLayout Layout() const noexcept { return _layouts[_layoutIndex]; }
        void SetLayouts(const std::array<ScreenLayout, config::screen::MAX_SCREEN_LAYOUTS>& layouts, unsigned numberOfLayouts) noexcept {
            ScreenLayout oldLayout = Layout();

            if (_layoutIndex >= numberOfLayouts) {
                _layoutIndex = numberOfLayouts - 1;
            }
            _layouts = layouts;
            _numberOfLayouts = numberOfLayouts;

            if (oldLayout != Layout()) _dirty = true;
        }

        void NextLayout() noexcept {
            ScreenLayout oldLayout = Layout();
            _layoutIndex = (_layoutIndex + 1) % _numberOfLayouts;

            if (oldLayout != Layout()) _dirty = true;
        }

        bool IsHybridLayout() const noexcept { return Layout() == ScreenLayout::HybridTop || Layout() == ScreenLayout::HybridBottom; }
        SmallScreenLayout HybridSmallScreenLayout() const noexcept { return hybrid_small_screen; }
        void HybridSmallScreenLayout(SmallScreenLayout _layout) noexcept {
            if (IsHybridLayout() && _layout != hybrid_small_screen) _dirty = true;
            hybrid_small_screen = _layout;
        }

        bool TopScreenEnabled() const noexcept { return Layout() != ScreenLayout::BottomOnly; }
        bool BottomScreenEnabled() const noexcept { return Layout() != ScreenLayout::TopOnly; }

        unsigned ScreenGap() const noexcept { return screen_gap_unscaled; }
        void ScreenGap(unsigned _screen_gap) noexcept {
            if (_screen_gap != screen_gap_unscaled) _dirty = true;
            screen_gap_unscaled = _screen_gap;
        }
        unsigned ScaledScreenGap() const noexcept { return screen_gap_unscaled * scale; }

        unsigned HybridRatio() const noexcept { return hybrid_ratio; }
        void HybridRatio(unsigned _hybrid_ratio) noexcept {
            if (IsHybridLayout() && _hybrid_ratio != hybrid_ratio) _dirty = true;
            hybrid_ratio = _hybrid_ratio;
        }

        unsigned TopScreenOffset() const noexcept { return top_screen_offset; }
        unsigned BottomScreenOffset() const noexcept { return bottom_screen_offset; }

        unsigned TouchOffsetX() const noexcept { return touch_offset_x; }
        unsigned TouchOffsetY() const noexcept { return touch_offset_y; }

        retro_game_geometry Geometry(Renderer renderer) const noexcept;
    private:
        bool _dirty;
        bool direct_copy;
        unsigned scale;

        glm::uvec2 screen_size;
        unsigned top_screen_offset;
        unsigned bottom_screen_offset;

        unsigned touch_offset_x;
        unsigned touch_offset_y;

        unsigned screen_gap_unscaled;

        SmallScreenLayout hybrid_small_screen;
        unsigned hybrid_ratio;

        unsigned _layoutIndex;
        unsigned _numberOfLayouts;
        std::array<ScreenLayout, config::screen::MAX_SCREEN_LAYOUTS> _layouts;

        // TODO: Move the buffer to a separate class
        unsigned buffer_width;
        unsigned buffer_height;
        unsigned buffer_stride;
        size_t buffer_len;
        uint16_t *buffer_ptr;
    };

    constexpr unsigned MaxSoftwareRenderedWidth() noexcept {
        using config::screen::MAX_SOFTWARE_HYBRID_RATIO;
        return std::max(
            // Left/Right or Right/Left layout
            NDS_SCREEN_WIDTH * 2u,

            // Hybrid layout
            (NDS_SCREEN_WIDTH * MAX_SOFTWARE_HYBRID_RATIO) + NDS_SCREEN_WIDTH + (MAX_SOFTWARE_HYBRID_RATIO * 2)
        );
    }

    constexpr unsigned MaxSoftwareRenderedHeight() noexcept {
        using namespace config::screen;
        return NDS_SCREEN_HEIGHT * 2 + MAX_SCREEN_GAP;
    }

    constexpr unsigned MaxOpenGlRenderedWidth() noexcept {
        using config::screen::MAX_HYBRID_RATIO;
        unsigned scale = config::video::MAX_OPENGL_SCALE;
        // TODO: What if this is too big?

        return std::max(
            // Left/Right or Right/Left layout
            NDS_SCREEN_WIDTH * scale * 2,

            // Hybrid layout
            (NDS_SCREEN_WIDTH * scale * MAX_HYBRID_RATIO) + (NDS_SCREEN_WIDTH * scale) + MAX_HYBRID_RATIO * 2
        );
    }

    constexpr unsigned MaxOpenGlRenderedHeight() noexcept {
        using namespace config::screen;
        unsigned scale = config::video::MAX_OPENGL_SCALE;

        return std::max(
            scale * (NDS_SCREEN_HEIGHT * 2 + MAX_SCREEN_GAP),
            scale * NDS_SCREEN_HEIGHT * MAX_HYBRID_RATIO
        );
    }
}
#endif //MELONDS_DS_SCREENLAYOUT_HPP
