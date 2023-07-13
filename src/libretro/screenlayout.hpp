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
#include <glm/trigonometric.hpp>

#include "config.hpp"
#include "environment.hpp"

namespace melonds {
    /// The native width of a single Nintendo DS screen, in pixels
    constexpr int NDS_SCREEN_WIDTH = 256;

    /// The native height of a single Nintendo DS screen, in pixels
    constexpr int NDS_SCREEN_HEIGHT = 192;

    template<typename T>
    constexpr glm::tvec2<T> NDS_SCREEN_SIZE(NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT);

    template<typename T>
    constexpr T NDS_SCREEN_AREA = NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT;

    // We require a pixel format of RETRO_PIXEL_FORMAT_XRGB8888, so we can assume 4 bytes here
    constexpr int PIXEL_SIZE = 4;

    template<typename T>
    constexpr T RETRO_MAX_POINTER_LENGTH = 32767;

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
        void CopyScreen(const uint32_t* src, unsigned offset) noexcept;
        void CopyHybridScreen(const uint32_t* src, HybridScreenId screen_id) noexcept;
        [[deprecated("Move to render.cpp")]] void DrawCursor(glm::ivec2 touch) noexcept;
        void Clear();

        void Update(Renderer renderer) noexcept;

        bool Dirty() const noexcept { return _dirty; }

        void* Buffer() noexcept { return buffer; }
        const void* Buffer() const noexcept { return buffer; }

        /// The width of the image necessary to hold this layout, in pixels
        unsigned BufferWidth() const noexcept { return bufferWidth; }

        /// The height of the image necessary to hold this layout, in pixels
        unsigned BufferHeight() const noexcept { return bufferHeight; }

        /// The size of the image necessary to hold this layout, in pixels
        glm::uvec2 BufferSize() const noexcept { return glm::uvec2(bufferWidth, bufferHeight); }
        unsigned BufferStride() const noexcept { return bufferStride; }
        float BufferAspectRatio() const noexcept {
            switch (Layout()) {
                case ScreenLayout::TurnLeft:
                case ScreenLayout::TurnRight:
                    return float(bufferHeight) / float(bufferWidth);
                default:
                    return float(bufferWidth) / float(bufferHeight);
            }
        }

        unsigned ScreenWidth() const noexcept { return screenSize.x; }
        unsigned ScreenHeight() const noexcept { return screenSize.y; }
        unsigned ScreenArea() const noexcept { return screenSize.x * screenSize.y; }
        float ScreenAspectRatio() const noexcept {
            switch (Layout()) {
                case ScreenLayout::TurnLeft:
                case ScreenLayout::TurnRight:
                    return float(screenSize.y) / float(screenSize.x);
                default:
                    return float(screenSize.x) / float(screenSize.y);
            }
        }
        glm::uvec2 ScreenSize() const noexcept { return screenSize; }

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
        SmallScreenLayout HybridSmallScreenLayout() const noexcept { return hybridSmallScreenLayout; }
        void HybridSmallScreenLayout(SmallScreenLayout _layout) noexcept {
            if (IsHybridLayout() && _layout != hybridSmallScreenLayout) _dirty = true;
            hybridSmallScreenLayout = _layout;
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

        unsigned ScreenGap() const noexcept { return screenGap; }
        void ScreenGap(unsigned _screen_gap) noexcept {
            if (_screen_gap != screenGap) _dirty = true;
            screenGap = _screen_gap;
        }
        unsigned ScaledScreenGap() const noexcept { return screenGap * scale; }
        unsigned Scale() const noexcept { return scale; }
        void SetScale(unsigned _scale) noexcept {
            if (_scale != scale) _dirty = true;
            scale = _scale;
        }

        unsigned HybridRatio() const noexcept { return hybridRatio; }
        void HybridRatio(unsigned _hybrid_ratio) noexcept {
            if (IsHybridLayout() && _hybrid_ratio != hybridRatio) _dirty = true;
            hybridRatio = _hybrid_ratio;
        }

        unsigned TopScreenBufferOffset() const noexcept { return topScreenBufferOffset; }
        unsigned BottomScreenBufferOffset() const noexcept { return bottomScreenBufferOffset; }
        unsigned ScreenBufferOffset(NdsScreenId screen) const noexcept { return screen == NdsScreenId::Top ? topScreenBufferOffset : bottomScreenBufferOffset; }

        [[deprecated("Use TransformInput instead")]] unsigned TouchOffsetX() const noexcept { return touchOffset.x; }
        [[deprecated("Use TransformInput instead")]] unsigned TouchOffsetY() const noexcept { return touchOffset.y; }

        /// @param input Coordinates in pointer space (from -32767 to 32767)
        [[nodiscard]] glm::ivec2 TransformInput(glm::ivec2 input) const noexcept {
            glm::vec3 transformed = transformMatrix * glm::vec3(input, 1.0f);
            return glm::ivec2(transformed.x, transformed.y);
        }

        retro_game_geometry Geometry(Renderer renderer) const noexcept;
    private:
        bool _dirty;
        unsigned scale;

        glm::uvec2 screenSize;
        glm::mat3 transformMatrix;

        // The starting point of the top screen's pixel data, in bytes.
        // Only used with the software renderer.
        size_t topScreenBufferOffset;

        // The starting point of the bottom screen's pixel data, in bytes
        // Only used with the software renderer.
        size_t bottomScreenBufferOffset;

        glm::uvec2 touchOffset;

        unsigned screenGap;

        SmallScreenLayout hybridSmallScreenLayout;
        unsigned hybridRatio;

        unsigned _layoutIndex;
        unsigned _numberOfLayouts;
        std::array<ScreenLayout, config::screen::MAX_SCREEN_LAYOUTS> _layouts;

        unsigned bufferWidth;
        unsigned bufferHeight;
        unsigned bufferStride;
        uint16_t *buffer;
    };

    constexpr bool LayoutSupportsDirectCopy(ScreenLayout layout) noexcept {
        switch (layout) {
            case ScreenLayout::TurnLeft:
            case ScreenLayout::TurnRight:
            case ScreenLayout::UpsideDown:
            case ScreenLayout::TopBottom:
            case ScreenLayout::BottomTop:
            case ScreenLayout::TopOnly:
            case ScreenLayout::BottomOnly:
                return true;
            default:
                return false;
        }
    }

    constexpr float LayoutAngle(ScreenLayout layout) noexcept {
        switch (layout) {
            case ScreenLayout::TurnLeft:
                return glm::radians(90.f);
            case ScreenLayout::TurnRight:
                return glm::radians(270.f);
            case ScreenLayout::UpsideDown:
                return glm::radians(180.f);
            default:
                return 0.0f;
        }
    }

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
