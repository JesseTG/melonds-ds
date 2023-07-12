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
#include <glm/mat3x3.hpp>

#include "config.hpp"
#include "environment.hpp"

namespace melonds {
    /// The native width of a single Nintendo DS screen, in pixels
    constexpr int NDS_SCREEN_WIDTH = 256;

    /// The native height of a single Nintendo DS screen, in pixels
    constexpr int NDS_SCREEN_HEIGHT = 192;

    // We require a pixel format of RETRO_PIXEL_FORMAT_XRGB8888, so we can assume 4 bytes here
    constexpr int PIXEL_SIZE = 4;

    enum class HybridScreenId {
        Top,
        Bottom,
        Primary,
    };

    enum class NdsScreenId {
        Top,
        Bottom,
    };

    class ScreenLayoutData {
    public:
        ScreenLayoutData();
        ~ScreenLayoutData();
        [[deprecated("Move to ScreenBuffer")]] void CopyScreen(const uint32_t* src, unsigned offset) noexcept;
        [[deprecated("Move to ScreenBuffer")]] void CopyHybridScreen(const uint32_t* src, HybridScreenId screen_id) noexcept;
        [[deprecated("Move to render.cpp")]] void draw_cursor(int32_t x, int32_t y);
        void Clear();

        void Update(Renderer renderer) noexcept;

        bool Dirty() const noexcept { return _dirty; }

        [[deprecated("Move to ScreenBuffer")]] void* Buffer() noexcept { return buffer_ptr; }
        [[deprecated("Move to ScreenBuffer")]] const void* Buffer() const noexcept { return buffer_ptr; }

        unsigned BufferWidth() const noexcept { return buffer_width; }
        unsigned BufferHeight() const noexcept { return buffer_height; }
        glm::uvec2 BufferSize() const noexcept { return glm::uvec2(buffer_width, buffer_height); }
        unsigned BufferStride() const noexcept { return buffer_stride; }
        float BufferAspectRatio() const noexcept {
            switch (Layout()) {
                case ScreenLayout::TurnLeft:
                case ScreenLayout::TurnRight:
                    return float(buffer_height) / float(buffer_width);
                default:
                    return float(buffer_width) / float(buffer_height);
            }
        }

        unsigned ScreenWidth() const noexcept { return screen_size.x; }
        unsigned ScreenHeight() const noexcept { return screen_size.y; }
        unsigned ScreenArea() const noexcept { return screen_size.x * screen_size.y; }
        float ScreenAspectRatio() const noexcept {
            switch (Layout()) {
                case ScreenLayout::TurnLeft:
                case ScreenLayout::TurnRight:
                    return float(screen_size.y) / float(screen_size.x);
                default:
                    return float(screen_size.x) / float(screen_size.y);
            }
        }
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

        bool IsLayoutRotated() const noexcept {
            switch (Layout()) {
                case ScreenLayout::TurnLeft:
                case ScreenLayout::TurnRight:
                case ScreenLayout::UpsideDown:
                    return true;
                default:
                    return false;
            }
        }

        retro::ScreenOrientation LayoutOrientation() const noexcept {
            switch (Layout()) {
                case ScreenLayout::TurnLeft:
                    return retro::ScreenOrientation::RotatedLeft;
                case ScreenLayout::TurnRight:
                    return retro::ScreenOrientation::RotatedRight;
                case ScreenLayout::UpsideDown:
                    return retro::ScreenOrientation::UpsideDown;
                default:
                    return retro::ScreenOrientation::Normal;
            }
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

        unsigned TopScreenBufferOffset() const noexcept { return top_screen_offset; }
        unsigned BottomScreenBufferOffset() const noexcept { return bottom_screen_offset; }

        unsigned TouchOffsetX() const noexcept { return touch_offset_x; }
        unsigned TouchOffsetY() const noexcept { return touch_offset_y; }

        retro_game_geometry Geometry(Renderer renderer) const noexcept;
    private:
        bool _dirty;
        [[deprecated("Move to ScreenBuffer")]] bool direct_copy;
        unsigned scale;

        glm::uvec2 screen_size;
        glm::mat3 transformMatrix;
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
        [[deprecated("Move to ScreenBuffer")]] uint16_t *buffer_ptr;
    };

    constexpr unsigned MaxSoftwareRenderedWidth() noexcept {
        using namespace config::screen;
        return std::max({
            // Left/Right or Right/Left layout
            NDS_SCREEN_WIDTH * 2u,

            // Hybrid layout
            (NDS_SCREEN_WIDTH * MAX_SOFTWARE_HYBRID_RATIO) + NDS_SCREEN_WIDTH + (MAX_SOFTWARE_HYBRID_RATIO * 2),

            // Sideways layout
            NDS_SCREEN_HEIGHT * 2 + MAX_SCREEN_GAP,
        });
    }

    constexpr unsigned MaxSoftwareRenderedHeight() noexcept {
        using namespace config::screen;
        return NDS_SCREEN_HEIGHT * 2 + MAX_SCREEN_GAP;
    }

    constexpr unsigned MaxOpenGlRenderedWidth() noexcept {
        using namespace config::screen;
        unsigned scale = config::video::MAX_OPENGL_SCALE;
        // TODO: What if this is too big?

        return std::max({
            // Left/Right or Right/Left layout
            NDS_SCREEN_WIDTH * scale * 2,

            // Hybrid layout
            (NDS_SCREEN_WIDTH * scale * MAX_HYBRID_RATIO) + (NDS_SCREEN_WIDTH * scale) + MAX_HYBRID_RATIO * 2,

            // Sideways layout
            scale * (NDS_SCREEN_HEIGHT * 2 + MAX_SCREEN_GAP),
        });
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
