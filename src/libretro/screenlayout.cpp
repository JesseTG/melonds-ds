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

//! The screen layout math in this file is derived from this Geogebra diagram I made: https://www.geogebra.org/m/rc2wpjax

#include "screenlayout.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <functional>

#include <glm/gtx/matrix_transform_2d.hpp>

#include "config.hpp"
#include "math.hpp"

using std::array;
using std::max;
using glm::inverse;
using glm::ivec2;
using glm::ivec3;
using glm::scale;
using glm::uvec2;
using glm::vec2;
using glm::vec3;
using glm::mat3;

melonds::ScreenLayoutData::ScreenLayoutData() :
    _dirty(true), // Uninitialized
    joystickMatrix(1), // Identity matrix
    topScreenMatrix(1),
    bottomScreenMatrix(1),
    bottomScreenMatrixInverse(1),
    hybridScreenMatrix(1),
    pointerMatrix(1),
    hybridRatio(2),
    _numberOfLayouts(1),
    buffer(nullptr),
    hybridBuffer(nullptr) {
}

melonds::ScreenLayoutData::~ScreenLayoutData() noexcept {
    scaler_ctx_gen_reset(&hybridScaler);
}

void melonds::ScreenLayoutData::CopyScreen(const uint32_t* src, glm::uvec2 destTranslation) noexcept {
    // Only used for software rendering

    // melonDS's software renderer draws each emulated screen to its own buffer,
    // and then the frontend combines them based on the current layout.
    // In the original buffer, all pixels are contiguous in memory.
    // If a screen doesn't need anything drawn to its side (such as blank space or another screen),
    // then we can just copy the entire screen at once.
    // But if a screen *does* need anything drawn on either side of it,
    // then its pixels can't all be contiguous in memory.
    // In that case, we have to copy each row of pixels individually to a different offset.
    if (LayoutSupportsDirectCopy(Layout())) {
        buffer.CopyDirect(src, destTranslation);
    } else {
        // Not all of this screen's pixels will be contiguous in memory, so we have to copy them row by row
        buffer.CopyRows(src, destTranslation, NDS_SCREEN_SIZE<unsigned>);
    }

}

void melonds::ScreenLayoutData::CopyHybridScreen(const uint32_t* src, HybridScreenId screen_id, glm::uvec2 destTranslation) noexcept {
    // Only used for software rendering

    switch (screen_id) {
        case HybridScreenId::Primary: {
            scaler_ctx_scale(&hybridScaler, hybridBuffer.Buffer(), src);
            buffer.CopyRows(hybridBuffer.Buffer(), destTranslation, NDS_SCREEN_SIZE<unsigned> * hybridRatio);
            break;
        }
        case HybridScreenId::Top: {
            buffer.CopyRows(src, destTranslation, NDS_SCREEN_SIZE<unsigned>);
            break;
        }
        case HybridScreenId::Bottom: {
            buffer.CopyRows(src, destTranslation, NDS_SCREEN_SIZE<unsigned>);
            break;
        }
    }
}

void melonds::ScreenLayoutData::DrawCursor(ivec2 touch, const mat3& matrix) noexcept {
    // Only used for software rendering
    if (!buffer)
        return;

    uint32_t* base_offset = buffer.Buffer();

    ivec2 clampedTouch = glm::clamp(touch, ivec2(0), ivec2(NDS_SCREEN_WIDTH - 1, NDS_SCREEN_HEIGHT - 1));
    ivec2 transformedTouch = matrix * vec3(clampedTouch, 1);

    float cursorSize = melonds::config::video::CursorSize();
    uint32_t start_y = std::clamp<uint32_t>(transformedTouch.y - cursorSize, 0, bufferSize.y - 1);
    uint32_t end_y = std::clamp<uint32_t>(transformedTouch.y + cursorSize, 0, bufferSize.y - 1);

    for (uint32_t y = start_y; y < end_y; y++) {
        uint32_t start_x = std::clamp<uint32_t>(transformedTouch.x - cursorSize, 0, bufferSize.x - 1);
        uint32_t end_x = std::clamp<uint32_t>(transformedTouch.x + cursorSize, 0, bufferSize.x - 1);

        for (uint32_t x = start_x; x < end_x; x++) {
            // TODO: Replace with SIMD (does GLM have a SIMD version of this?)
            uint32_t *offset = base_offset + (y * bufferSize.x) + x;
            uint32_t pixel = *offset;
            *(uint32_t *) offset = (0xFFFFFF - pixel) | 0xFF000000;
        }
    }
}

void melonds::ScreenLayoutData::CombineScreens(const uint32_t* topBuffer, const uint32_t* bottomBuffer, const InputState& input) noexcept {
    if (!buffer)
        return;

    Clear();
    ScreenLayout layout = Layout();
    if (IsHybridLayout(layout)) {
        const uint32_t* primaryBuffer = layout == ScreenLayout::HybridTop ? topBuffer : bottomBuffer;

        CopyHybridScreen(primaryBuffer, HybridScreenId::Primary, hybridScreenTranslation);

        HybridSideScreenDisplay smallScreenLayout = HybridSmallScreenLayout();

        if (smallScreenLayout == HybridSideScreenDisplay::Both || layout == ScreenLayout::HybridBottom) {
            // If we should display both screens, or if the bottom one is the primary...
            CopyHybridScreen(topBuffer, HybridScreenId::Top, topScreenTranslation);
        }

        if (smallScreenLayout == HybridSideScreenDisplay::Both || layout == ScreenLayout::HybridTop) {
            // If we should display both screens, or if the top one is being focused...
            CopyHybridScreen(bottomBuffer, HybridScreenId::Bottom, bottomScreenTranslation);
        }

        if (input.CursorEnabled()) {
            DrawCursor(input.TouchPosition(), bottomScreenMatrix);
            DrawCursor(input.TouchPosition(), hybridScreenMatrix);
        }

    } else {
        if (layout != ScreenLayout::BottomOnly)
            CopyScreen(topBuffer, topScreenTranslation);

        if (layout != ScreenLayout::TopOnly)
            CopyScreen(bottomBuffer, bottomScreenTranslation);

        if (input.CursorEnabled() && layout != ScreenLayout::TopOnly)
            DrawCursor(input.TouchPosition(), bottomScreenMatrix);
    }
}

/// For a screen in the top left corner
mat3 NorthwestMatrix(unsigned resolutionScale) noexcept {
    return scale(mat3(1), vec2(resolutionScale));
}

/// For a screen on the bottom that accounts for the screen gap
constexpr mat3 SouthwestMatrix(unsigned resolutionScale, unsigned screenGap) noexcept {
    using namespace melonds;
    return math::ts<float>(
        vec2(0, resolutionScale * (NDS_SCREEN_HEIGHT + screenGap)),
        vec2(resolutionScale)
    );
}

/// For a screen on the right
constexpr mat3 EastMatrix(unsigned resolutionScale) noexcept {
    using namespace melonds;
    return math::ts<float>(
        vec2(resolutionScale * NDS_SCREEN_WIDTH, 0),
        vec2(resolutionScale)
    );
}

/// For the northeast hybrid screen
constexpr mat3 HybridWestMatrix(unsigned resolutionScale, unsigned hybridRatio) noexcept {
    using namespace melonds;
    return math::ts<float>(
        vec2(0),
        vec2(resolutionScale * hybridRatio)
    );
}

/// For the northeast hybrid screen
constexpr mat3 HybridNortheastMatrix(unsigned resolutionScale, unsigned hybridRatio) noexcept {
    using namespace melonds;
    return math::ts<float>(
        vec2(resolutionScale * hybridRatio * NDS_SCREEN_WIDTH, 0),
        vec2(resolutionScale)
    );
}

/// For the southeast hybrid screen
constexpr mat3 HybridSoutheastMatrix(unsigned resolutionScale, unsigned hybridRatio) noexcept {
    using namespace melonds;
    return math::ts<float>(
        vec2(resolutionScale * hybridRatio * NDS_SCREEN_WIDTH, resolutionScale * NDS_SCREEN_HEIGHT * (hybridRatio - 1)),
        vec2(resolutionScale)
    );
}

mat3 melonds::ScreenLayoutData::GetTopScreenMatrix(unsigned scale) const noexcept {
    switch (Layout()) {
        case ScreenLayout::TopBottom:
        case ScreenLayout::TopOnly:
        case ScreenLayout::LeftRight:
        case ScreenLayout::TurnLeft:
        case ScreenLayout::TurnRight:
        case ScreenLayout::UpsideDown:
            return NorthwestMatrix(scale);
        case ScreenLayout::BottomTop:
            return SouthwestMatrix(scale, screenGap);
        case ScreenLayout::RightLeft:
            return EastMatrix(scale);
        case ScreenLayout::HybridTop:
        case ScreenLayout::HybridBottom:
            return HybridNortheastMatrix(scale, hybridRatio);
        default:
            return mat3(1);
    }
}

mat3 melonds::ScreenLayoutData::GetBottomScreenMatrix(unsigned scale) const noexcept {
    switch (Layout()) {
        case ScreenLayout::TopBottom:
        case ScreenLayout::TurnLeft:
        case ScreenLayout::TurnRight:
        case ScreenLayout::UpsideDown:
            return SouthwestMatrix(scale, screenGap);
        case ScreenLayout::BottomTop:
        case ScreenLayout::BottomOnly:
        case ScreenLayout::RightLeft:
            return NorthwestMatrix(scale);
        case ScreenLayout::LeftRight:
            return EastMatrix(scale);
        case ScreenLayout::HybridTop:
        case ScreenLayout::HybridBottom:
            return HybridSoutheastMatrix(scale, hybridRatio);
        default:
            return mat3(1);
    }
}

glm::mat3 melonds::ScreenLayoutData::GetHybridScreenMatrix(unsigned scale) const noexcept {
    switch (Layout()) {
        case ScreenLayout::HybridBottom:
        case ScreenLayout::HybridTop:
            return HybridWestMatrix(scale, hybridRatio);
        default:
            return mat3(1);
    }
}

void melonds::ScreenLayoutData::Update(melonds::Renderer renderer) noexcept {
    unsigned scale = (renderer == Renderer::Software) ? 1 : resolutionScale;
    uvec2 oldBufferSize = bufferSize;

    // These points represent the NDS screen coordinates without transformations
    array<vec2, 4> baseScreenPoints = {
        vec2(0, 0), // northwest
        vec2(NDS_SCREEN_WIDTH, 0), // northeast
        vec2(NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT), // southeast
        vec2(0, NDS_SCREEN_HEIGHT) // southwest
    };

    // Get the matrices we'll be using
    // (except the pointer matrix, we need to compute the buffer size first)
    topScreenMatrix = GetTopScreenMatrix(scale);
    bottomScreenMatrix = GetBottomScreenMatrix(scale);
    hybridScreenMatrix = GetHybridScreenMatrix(scale);
    hybridScreenMatrixInverse = inverse(hybridScreenMatrix);
    bottomScreenMatrixInverse = inverse(bottomScreenMatrix);

    // Transform the base screen points
    transformedScreenPoints = {
        topScreenMatrix * vec3(baseScreenPoints[0], 1),
        topScreenMatrix * vec3(baseScreenPoints[1], 1),
        topScreenMatrix * vec3(baseScreenPoints[2], 1),
        topScreenMatrix * vec3(baseScreenPoints[3], 1),
        bottomScreenMatrix * vec3(baseScreenPoints[0], 1),
        bottomScreenMatrix * vec3(baseScreenPoints[1], 1),
        bottomScreenMatrix * vec3(baseScreenPoints[2], 1),
        bottomScreenMatrix * vec3(baseScreenPoints[3], 1),
        hybridScreenMatrix * vec3(baseScreenPoints[0], 1),
        hybridScreenMatrix * vec3(baseScreenPoints[1], 1),
        hybridScreenMatrix * vec3(baseScreenPoints[2], 1),
        hybridScreenMatrix * vec3(baseScreenPoints[3], 1)
    };

    // We need to compute the buffer size to use it for rendering and the touch screen
    bufferSize = uvec2(0);
    for (const vec2& p : transformedScreenPoints) {
        bufferSize.x = max<unsigned>(bufferSize.x, p.x);
        bufferSize.y = max<unsigned>(bufferSize.y, p.y);
    }

    topScreenTranslation = transformedScreenPoints[0];
    bottomScreenTranslation = transformedScreenPoints[4];
    hybridScreenTranslation = transformedScreenPoints[8];
    pointerMatrix = math::ts<float>(vec2(bufferSize) / 2.0f, vec2(bufferSize) / (2.0f * RETRO_MAX_POINTER_COORDINATE<float>));

    ScreenLayout layout = Layout();
    pointerMatrix = glm::rotate(pointerMatrix, LayoutAngle(layout));
    if (!retro::set_screen_rotation(LayoutOrientation(layout))) {
        // TODO: Rotate the screen outside screenlayout, but _before_ update;
        // if it failed, handle it accordingly in update
    }

    if (renderer == Renderer::OpenGl) {
        // not needed anymore :)
        buffer = nullptr;
        hybridBuffer = nullptr;
    } else {
        if (bufferSize != oldBufferSize || !buffer) {
            buffer = PixelBuffer(bufferSize);
        }

        if (IsHybridLayout(Layout())) {
            // TODO: Don't recreate this buffer if the hybrid ratio didn't change
            // TODO: Maintain a separate _hybridDirty flag
            hybridBuffer = PixelBuffer(NDS_SCREEN_SIZE<unsigned> * hybridRatio);
            hybridScaler.in_width = NDS_SCREEN_WIDTH;
            hybridScaler.in_height = NDS_SCREEN_HEIGHT;
            hybridScaler.in_stride = NDS_SCREEN_WIDTH * sizeof(uint32_t);
            hybridScaler.out_width = NDS_SCREEN_WIDTH * hybridRatio;
            hybridScaler.out_height = NDS_SCREEN_HEIGHT * hybridRatio;
            hybridScaler.out_stride = NDS_SCREEN_WIDTH * hybridRatio * sizeof(uint32_t);
            hybridScaler.in_fmt = SCALER_FMT_ARGB8888;
            hybridScaler.out_fmt = SCALER_FMT_ARGB8888;
            hybridScaler.scaler_type = SCALER_TYPE_POINT;
            scaler_ctx_gen_filter(&hybridScaler);
        } else {
            hybridBuffer = nullptr;
        }
    }

    _dirty = false;
}


void melonds::ScreenLayoutData::Clear() noexcept {
    if (buffer) {
        buffer.Clear();
    }
}


retro_game_geometry melonds::ScreenLayoutData::Geometry(melonds::Renderer renderer) const noexcept {
    retro_game_geometry geometry {
        .base_width = BufferWidth(),
        .base_height = BufferHeight(),
        .max_width = MaxSoftwareRenderedWidth(),
        .max_height = MaxSoftwareRenderedHeight(),
        .aspect_ratio = BufferAspectRatio(),
    };

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (renderer == Renderer::OpenGl) {
        geometry.max_width = MaxOpenGlRenderedWidth();
        geometry.max_height = MaxOpenGlRenderedHeight();
    }
#endif

    return geometry;
}
