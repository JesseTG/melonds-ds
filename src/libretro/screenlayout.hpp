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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <libretro.h>
#include <gfx/scaler/scaler.h>

#include <glm/vec2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/trigonometric.hpp>

#include "config/constants.hpp"
#include "environment.hpp"
#include "input.hpp"
#include "buffer.hpp"
#include "retro/scaler.hpp"

namespace melonDS {
    class Renderer3D;
}

namespace MelonDsDs {
    class RenderState;
    class RenderStateWrapper;

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
    constexpr T RETRO_MAX_POINTER_COORDINATE = 32767;

    enum class HybridScreenId {
        Top,
        Bottom,
        Primary,
    };

    enum class NdsScreenId {
        Top,
        Bottom,
    };

    constexpr bool IsHybridLayout(ScreenLayout layout) noexcept {
        switch (layout) {
            case ScreenLayout::HybridTop:
            case ScreenLayout::HybridBottom:
                return true;
            default:
                return false;
        }
    }

    class ScreenLayoutData {
    public:
        ScreenLayoutData();
        ~ScreenLayoutData() noexcept;

        void Apply(const CoreConfig& config, const RenderStateWrapper& renderState) noexcept;
        void Update() noexcept;

        bool Dirty() const noexcept { return _dirty; }

        /// The width of the image necessary to hold this layout, in pixels
        unsigned BufferWidth() const noexcept { return bufferSize.x; }

        /// The height of the image necessary to hold this layout, in pixels
        unsigned BufferHeight() const noexcept { return bufferSize.y; }

        /// The size of the image necessary to hold this layout, in pixels
        glm::uvec2 BufferSize() const noexcept { return bufferSize; }

        float BufferAspectRatio() const noexcept {
            switch (Layout()) {
                case ScreenLayout::TurnLeft:
                case ScreenLayout::TurnRight:
                    return float(BufferHeight()) / float(BufferWidth());
                default:
                    return float(BufferWidth()) / float(BufferHeight());
            }
        }

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

        void SetLayouts(const std::span<const ScreenLayout> layouts) noexcept {
            ScreenLayout oldLayout = Layout();

            if (_layoutIndex >= layouts.size()) {
                _layoutIndex = layouts.size() - 1;
            }
            memcpy(_layouts.data(), layouts.data(), layouts.size() * sizeof(ScreenLayout));
            _numberOfLayouts = layouts.size();

            if (oldLayout != Layout()) _dirty = true;
        }

        void NextLayout() noexcept {
            ScreenLayout oldLayout = Layout();
            _layoutIndex = (_layoutIndex + 1) % _numberOfLayouts;

            if (oldLayout != Layout()) _dirty = true;
        }

        HybridSideScreenDisplay HybridSmallScreenLayout() const noexcept { return hybridSmallScreenLayout; }
        void HybridSmallScreenLayout(HybridSideScreenDisplay _layout) noexcept {
            if (IsHybridLayout(Layout()) && _layout != hybridSmallScreenLayout) _dirty = true;
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

        unsigned ScreenGap() const noexcept { return screenGap; }
        void ScreenGap(unsigned _screen_gap) noexcept {
            if (_screen_gap != screenGap) _dirty = true;
            screenGap = _screen_gap;
        }

        unsigned Scale() const noexcept { return resolutionScale; }
        void SetScale(unsigned _scale) noexcept {
            if (_scale != resolutionScale) _dirty = true;
            resolutionScale = _scale;
        }

        unsigned HybridRatio() const noexcept { return hybridRatio; }
        void HybridRatio(unsigned _hybrid_ratio) noexcept {
            if (IsHybridLayout(Layout()) && _hybrid_ratio != hybridRatio) _dirty = true;
            hybridRatio = _hybrid_ratio;
        }

        /// @param input Coordinates in pointer space (from -32767 to 32767)
        [[nodiscard]] glm::ivec2 TransformPointerInput(glm::i16vec2 input) const noexcept {
            glm::vec3 transformed = bottomScreenMatrixInverse * pointerMatrix * glm::vec3(input, 1.0f);
            return glm::ivec2(transformed.x, transformed.y);
        }

        [[nodiscard]] glm::ivec2 TransformPointerInput(int16_t x, int16_t y) const noexcept {
            return TransformPointerInput(glm::i16vec2(x, y));
        }

        /// @param input Coordinates in pointer space (from -32767 to 32767)
        [[nodiscard]] glm::ivec2 TransformPointerInputToHybridScreen(glm::i16vec2 input) const noexcept {
            glm::vec3 transformed = hybridScreenMatrixInverse * pointerMatrix * glm::vec3(input, 1.0f);
            return glm::ivec2(transformed.x, transformed.y);
        }

        [[nodiscard]] glm::ivec2 TransformPointerInputToHybridScreen(int16_t x, int16_t y) const noexcept {
            return TransformPointerInputToHybridScreen(glm::i16vec2(x, y));
        }

        [[nodiscard]] const std::array<glm::vec2, 12>& TransformedScreenPoints() const noexcept {
            return transformedScreenPoints;
        }

        [[nodiscard]] retro_game_geometry Geometry(const melonDS::Renderer3D& renderer) const noexcept;

        [[nodiscard]] retro::ScreenOrientation EffectiveOrientation() const noexcept { return orientation; }
        [[nodiscard]] const glm::mat3& GetBottomScreenMatrix() const noexcept { return bottomScreenMatrix; }
        [[nodiscard]] glm::uvec2 GetTopScreenTranslation() const noexcept { return topScreenTranslation; }
        [[nodiscard]] glm::uvec2 GetBottomScreenTranslation() const noexcept { return bottomScreenTranslation; }
        [[nodiscard]] glm::uvec2 GetHybridScreenTranslation() const noexcept { return hybridScreenTranslation; }
    private:
        glm::mat3 GetTopScreenMatrix(unsigned scale) const noexcept;
        glm::mat3 GetBottomScreenMatrix(unsigned scale) const noexcept;
        glm::mat3 GetHybridScreenMatrix(unsigned scale) const noexcept;

        bool _dirty;
        unsigned resolutionScale;
        retro::ScreenOrientation orientation;
        std::array<glm::vec2, 12> transformedScreenPoints;

        glm::mat3 joystickMatrix;
        glm::mat3 topScreenMatrix;
        glm::mat3 bottomScreenMatrix;
        glm::mat3 bottomScreenMatrixInverse;
        glm::mat3 hybridScreenMatrix;
        glm::mat3 hybridScreenMatrixInverse;
        glm::mat3 pointerMatrix;

        unsigned screenGap;

        HybridSideScreenDisplay hybridSmallScreenLayout;
        unsigned hybridRatio;

        unsigned _layoutIndex;
        unsigned _numberOfLayouts;
        std::array<ScreenLayout, config::screen::MAX_SCREEN_LAYOUTS> _layouts;

        // Offset in pixels, not bytes
        glm::uvec2 topScreenTranslation;
        glm::uvec2 bottomScreenTranslation;
        glm::uvec2 hybridScreenTranslation;

        glm::uvec2 bufferSize;
    };

    constexpr bool LayoutSupportsScreenGap(ScreenLayout layout) noexcept {
        switch (layout) {
            case ScreenLayout::TurnLeft:
            case ScreenLayout::TurnRight:
            case ScreenLayout::UpsideDown:
            case ScreenLayout::TopBottom:
            case ScreenLayout::BottomTop:
                return true;
            default:
                return false;
        }
    }

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

    constexpr retro::ScreenOrientation LayoutOrientation(ScreenLayout layout) noexcept {
        switch (layout) {
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
            (NDS_SCREEN_WIDTH * MAX_HYBRID_RATIO) + NDS_SCREEN_WIDTH + (MAX_HYBRID_RATIO * 2),

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
