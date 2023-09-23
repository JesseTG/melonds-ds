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

#include <glm/gtx/matrix_transform_2d.hpp>
#include <retro_assert.h>

#include "config.hpp"
#include "math.hpp"
#include "tracy.hpp"

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
}

void melonds::ScreenLayoutData::CopyScreen(const uint32_t* src, glm::uvec2 destTranslation) noexcept {
    ZoneScopedN("melonds::ScreenLayoutData::CopyScreen");
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

void melonds::ScreenLayoutData::DrawCursor(glm::ivec2 touch) noexcept {
    switch (Layout()) {
        default:
            DrawCursor(touch, bottomScreenMatrix);
            break;
        case ScreenLayout::TopOnly:
            return;
    }
}

void melonds::ScreenLayoutData::DrawCursor(ivec2 touch, const mat3& matrix) noexcept {
    ZoneScopedN("melonds::ScreenLayoutData::DrawCursor");
    // Only used for software rendering
    if (!buffer)
        return;

    ivec2 clampedTouch = glm::clamp(touch, ivec2(0), ivec2(NDS_SCREEN_WIDTH - 1, NDS_SCREEN_HEIGHT - 1));
    ivec2 transformedTouch = matrix * vec3(clampedTouch, 1);

    float cursorSize = melonds::config::screen::CursorSize();
    uvec2 start = glm::clamp(transformedTouch - ivec2(cursorSize), ivec2(0), ivec2(bufferSize));
    uvec2 end = glm::clamp(transformedTouch + ivec2(cursorSize), ivec2(0), ivec2(bufferSize));

    for (uint32_t y = start.y; y < end.y; y++) {
        for (uint32_t x = start.x; x < end.x; x++) {
            // TODO: Replace with SIMD (does GLM have a SIMD version of this?)
            uint32_t& pixel = buffer[uvec2(x, y)];
            pixel = (0xFFFFFF - pixel) | 0xFF000000;
        }
    }
}

void melonds::ScreenLayoutData::CombineScreens(const uint32_t* topBuffer, const uint32_t* bottomBuffer) noexcept {
    ZoneScopedN("melonds::ScreenLayoutData::CombineScreens");
    if (!buffer)
        return;

    Clear();
    ScreenLayout layout = Layout();
    if (IsHybridLayout(layout)) {
        retro_assert(hybridBuffer);
        const uint32_t* primaryBuffer = layout == ScreenLayout::HybridTop ? topBuffer : bottomBuffer;

        hybridScaler.Scale(hybridBuffer.Buffer(), primaryBuffer);
        buffer.CopyRows(hybridBuffer.Buffer(), hybridScreenTranslation, NDS_SCREEN_SIZE<unsigned> * hybridRatio);

        HybridSideScreenDisplay smallScreenLayout = HybridSmallScreenLayout();

        if (smallScreenLayout == HybridSideScreenDisplay::Both || layout == ScreenLayout::HybridBottom) {
            // If we should display both screens, or if the bottom one is the primary...
            buffer.CopyRows(topBuffer, topScreenTranslation, NDS_SCREEN_SIZE<unsigned>);
        }

        if (smallScreenLayout == HybridSideScreenDisplay::Both || layout == ScreenLayout::HybridTop) {
            // If we should display both screens, or if the top one is being focused...
            buffer.CopyRows(bottomBuffer, bottomScreenTranslation, NDS_SCREEN_SIZE<unsigned>);
        }

    } else {
        if (layout != ScreenLayout::BottomOnly)
            CopyScreen(topBuffer, topScreenTranslation);

        if (layout != ScreenLayout::TopOnly)
            CopyScreen(bottomBuffer, bottomScreenTranslation);
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
    ZoneScopedN("melonds::ScreenLayoutData::GetTopScreenMatrix");
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
    ZoneScopedN("melonds::ScreenLayoutData::GetBottomScreenMatrix");
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
    ZoneScopedN("melonds::ScreenLayoutData::GetHybridScreenMatrix");
    switch (Layout()) {
        case ScreenLayout::HybridBottom:
        case ScreenLayout::HybridTop:
            return HybridWestMatrix(scale, hybridRatio);
        default:
            return mat3(1);
    }
}

void melonds::ScreenLayoutData::Update(melonds::Renderer renderer) noexcept {
    ZoneScopedN("melonds::ScreenLayoutData::Update");
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
    retro::ScreenOrientation orientation = LayoutOrientation(layout);

    if (retro::set_screen_rotation(orientation)) {
        // Try to rotate the screen. If that failed...
        pointerMatrix = glm::rotate(pointerMatrix, LayoutAngle(layout));
    } else if (orientation != retro::ScreenOrientation::Normal) {
        // A rotation to normal orientation may "fail", even though it's the default.
        // So only log an error if we're trying to rotate to something besides 0 degrees.
        retro::set_error_message("Failed to rotate screen.");
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
            hybridScaler = retro::Scaler(
                SCALER_FMT_ARGB8888,
                SCALER_FMT_ARGB8888,
                config::video::ScreenFilter() == ScreenFilter::Nearest ? SCALER_TYPE_POINT : SCALER_TYPE_BILINEAR,
                NDS_SCREEN_WIDTH,
                NDS_SCREEN_HEIGHT,
                NDS_SCREEN_WIDTH * hybridRatio,
                NDS_SCREEN_HEIGHT * hybridRatio
            );
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
