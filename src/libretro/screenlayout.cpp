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
using glm::ivec2;
using glm::ivec3;
using glm::scale;
using glm::uvec2;
using glm::vec2;
using glm::vec3;
using glm::mat3;

melonds::ScreenLayoutData::ScreenLayoutData() :
    _dirty(true), // Uninitialized
    touchMatrix(1), // Identity matrix
    joystickMatrix(1),
    topScreenMatrix(1),
    bottomScreenMatrix(1),
    hybridScreenMatrix(1),
    hybridRatio(2),
    _numberOfLayouts(1),
    buffer(nullptr) {
}

melonds::ScreenLayoutData::~ScreenLayoutData() {
    free(buffer);
}

void melonds::ScreenLayoutData::CopyScreen(const uint32_t* src, unsigned offset) noexcept {
    // Only used for software rendering

    // melonDS's software renderer draws each emulated screen to its own buffer,
    // and then the frontend combines them based on the current layout.
    // In the original buffer, all pixels are contiguous in memory.
    // If a screen doesn't need anything drawn to its side (such as blank space or another screen),
    // then we can just copy the entire screen at once.
    // But if a screen *does* need anything drawn on either side of it,
    // then we have to copy each row of pixels individually to a different offset.
    if (LayoutSupportsDirectCopy(Layout())) {
        memcpy((uint32_t *) buffer + offset, src, NDS_SCREEN_AREA<size_t> * PIXEL_SIZE);
    } else {
        // Not all of this screen's pixels will be contiguous in memory, so we have to copy them row by row
        for (unsigned y = 0; y < NDS_SCREEN_HEIGHT; y++) {
            // For each row of the rendered screen...
            memcpy(
                (uint16_t *) buffer + offset + (y * NDS_SCREEN_WIDTH * PIXEL_SIZE),
                src + (y * NDS_SCREEN_WIDTH),
                NDS_SCREEN_WIDTH * PIXEL_SIZE
            );
        }
    }

}

void melonds::ScreenLayoutData::CopyHybridScreen(const uint32_t* src, HybridScreenId screen_id) noexcept {
    // Only used for software rendering
    switch (screen_id) {
        case HybridScreenId::Primary: {
            // TODO: Use one of the libretro scalers instead of this loop
            unsigned buffer_height = NDS_SCREEN_HEIGHT * hybridRatio;
            unsigned buffer_width = NDS_SCREEN_WIDTH * hybridRatio;

            for (unsigned buffer_y = 0; buffer_y < buffer_height; buffer_y++) {
                unsigned y = buffer_y / hybridRatio;
                for (unsigned buffer_x = 0; buffer_x < buffer_width; buffer_x++) {
                    unsigned x = buffer_x / hybridRatio;

                    uint32_t pixel_data = *(uint32_t *) (src + (y * NDS_SCREEN_WIDTH) + x);

                    for (unsigned pixel = 0; pixel < hybridRatio; pixel++) {
                        *(uint32_t *) (buffer + (buffer_y * bufferStride / 2) + pixel * 2 +
                                       (buffer_x * 2)) = pixel_data;
                    }
                }
            }

            break;
        }
        case HybridScreenId::Top: {
            for (unsigned y = 0; y < NDS_SCREEN_HEIGHT; y++) {
                memcpy((uint16_t *) buffer
                       // X
                       + ((NDS_SCREEN_WIDTH * hybridRatio * 2) +
                          (hybridRatio % 2 == 0 ? hybridRatio : ((hybridRatio / 2) * 4)))
                       // Y
                       + (y * bufferStride / 2),
                       src + (y * NDS_SCREEN_WIDTH),
                       NDS_SCREEN_WIDTH * PIXEL_SIZE
               );
            }

            break;
        }
        case HybridScreenId::Bottom: {
            for (unsigned y = 0; y < NDS_SCREEN_HEIGHT; y++) {
                memcpy((uint16_t *) buffer
                       // X
                       + ((NDS_SCREEN_WIDTH * hybridRatio * 2) +
                          (hybridRatio % 2 == 0 ? hybridRatio : ((hybridRatio / 2) * 4)))
                       // Y
                       + ((y + (NDS_SCREEN_HEIGHT * (hybridRatio - 1))) * bufferStride / 2),
                       src + (y * NDS_SCREEN_WIDTH),
                       NDS_SCREEN_WIDTH * PIXEL_SIZE);
            }

            break;
        }
    }
}

void melonds::ScreenLayoutData::DirectCopy(const uint32_t* source, unsigned offset) noexcept {

    memcpy((uint32_t *) buffer + offset, src, NDS_SCREEN_AREA<size_t> * PIXEL_SIZE);
}


void melonds::ScreenLayoutData::DrawCursor(ivec2 touch) noexcept {
    // Only used for software rendering
    if (!buffer)
        return;

    auto *base_offset = (uint32_t *) buffer;

    uint32_t scale = Layout() == ScreenLayout::HybridBottom ? hybridRatio : 1;
    float cursorSize = melonds::config::video::CursorSize();
    uint32_t start_y = std::clamp<float>(touch.y - cursorSize, 0, NDS_SCREEN_HEIGHT) * scale;
    uint32_t end_y = std::clamp<float>(touch.y + cursorSize, 0, NDS_SCREEN_HEIGHT) * scale;

    for (uint32_t y = start_y; y < end_y; y++) {
        uint32_t start_x = std::clamp<float>(touch.x - cursorSize, 0, NDS_SCREEN_WIDTH) * scale;
        uint32_t end_x = std::clamp<float>(touch.x + cursorSize, 0, NDS_SCREEN_WIDTH) * scale;

        for (uint32_t x = start_x; x < end_x; x++) {
            // TODO: Transform the coordinates instead of doing this
            uint32_t *offset = base_offset + ((y + touchOffset.y) * bufferWidth) + ((x + touchOffset.x));
            uint32_t pixel = *offset;
            *(uint32_t *) offset = (0xFFFFFF - pixel) | 0xFF000000;
        }
    }
}


void melonds::ScreenLayoutData::CombineScreens(const uint32_t* topBuffer, const uint32_t* bottomBuffer, std::optional<glm::ivec2> touch) noexcept {
    if (!buffer)
        return;

    switch (Layout()) {
        case ScreenLayout::TopBottom:
    }

}


void melonds::ScreenLayoutData::Clear() {
    if (buffer != nullptr) {
        memset(buffer, 0, bufferLength);
    }
}

using melonds::ScreenLayoutData;


/// For a screen in the top left corner
constexpr mat3 NorthwestMatrix(unsigned resolutionScale) noexcept {
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

mat3 melonds::ScreenLayoutData::GetTopScreenMatrix() const noexcept {
    switch (Layout()) {
        case ScreenLayout::TopBottom:
        case ScreenLayout::TopOnly:
        case ScreenLayout::LeftRight:
        case ScreenLayout::TurnLeft:
        case ScreenLayout::TurnRight:
        case ScreenLayout::UpsideDown:
            return NorthwestMatrix(resolutionScale);
        case ScreenLayout::BottomTop:
            return SouthwestMatrix(resolutionScale, screenGap);
        case ScreenLayout::RightLeft:
            return EastMatrix(resolutionScale);
        case ScreenLayout::HybridTop:
        case ScreenLayout::HybridBottom:
            return HybridNortheastMatrix(resolutionScale, hybridRatio);
        default:
            return mat3(1);
    }
}

mat3 melonds::ScreenLayoutData::GetBottomScreenMatrix() const noexcept {
    switch (Layout()) {
        case ScreenLayout::TopBottom:
        case ScreenLayout::TurnLeft:
        case ScreenLayout::TurnRight:
        case ScreenLayout::UpsideDown:
            return SouthwestMatrix(resolutionScale, screenGap);
        case ScreenLayout::BottomTop:
        case ScreenLayout::BottomOnly:
        case ScreenLayout::RightLeft:
            return NorthwestMatrix(resolutionScale);
        case ScreenLayout::LeftRight:
            return EastMatrix(resolutionScale);
        case ScreenLayout::HybridTop:
        case ScreenLayout::HybridBottom:
            return HybridSoutheastMatrix(resolutionScale, hybridRatio);
        default:
            return mat3(1);
    }
}

glm::mat3 melonds::ScreenLayoutData::GetHybridScreenMatrix() const noexcept {
    switch (Layout()) {
        case ScreenLayout::HybridBottom:
        case ScreenLayout::HybridTop:
            return HybridWestMatrix(resolutionScale, hybridRatio);
        default:
            return mat3(1);
    }
}

void melonds::ScreenLayoutData::Update(melonds::Renderer renderer) noexcept {
    size_t oldBufferLength = bufferLength;

    // These points represent the NDS screen coordinates without transformations
    array<vec2, 4> baseScreenPoints = {
        vec2(0, 0), // northwest
        vec2(NDS_SCREEN_WIDTH, 0), // northeast
        vec2(NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT), // southeast
        vec2(0, NDS_SCREEN_HEIGHT) // southwest
    };

    // Get the matrices we'll be using
    topScreenMatrix = GetTopScreenMatrix();
    bottomScreenMatrix = GetBottomScreenMatrix();
    hybridScreenMatrix = GetHybridScreenMatrix();

    // Transform the base screen points
    array<vec2, 12> transformedScreenPoints = {
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
    bufferStride = bufferSize.x * PIXEL_SIZE;
    bufferLength = bufferSize.x * bufferSize.y * PIXEL_SIZE;

    // TODO: Compute topScreenBufferOffset and bottomScreenBufferOffset

    ScreenLayout layout = Layout();
//    switch (layout) {
//        case ScreenLayout::TurnLeft:
//        case ScreenLayout::TurnRight:
//        case ScreenLayout::UpsideDown:
//        case ScreenLayout::TopBottom:
//            bufferWidth = NDS_SCREEN_WIDTH * scale;
//            bufferHeight = (NDS_SCREEN_HEIGHT * 2 + screenGap) * scale;
//            bufferStride = NDS_SCREEN_WIDTH * PIXEL_SIZE * scale;
//            touchOffset = uvec2(0, NDS_SCREEN_HEIGHT + screenGap);// * scale;
//            topScreenBufferOffset = 0;
//            bottomScreenBufferOffset = bufferWidth * (NDS_SCREEN_HEIGHT + screenGap) * scale;
//            // We rely on the libretro frontend to rotate the screen for us,
//            // so we don't have to treat the buffer dimensions specially here
//
//            break;
//        case ScreenLayout::BottomTop:
//            bufferWidth = NDS_SCREEN_WIDTH * scale;
//            bufferHeight = (NDS_SCREEN_HEIGHT * 2 + screenGap) * scale;
//            bufferStride = NDS_SCREEN_WIDTH * PIXEL_SIZE * scale;
//            touchOffset = uvec2(0, 0);
//            topScreenBufferOffset = bufferWidth * (NDS_SCREEN_HEIGHT + screenGap) * scale;
//            bottomScreenBufferOffset = 0;
//
//            break;
//        case ScreenLayout::LeftRight:
//            bufferWidth = NDS_SCREEN_WIDTH * 2 * scale;
//            bufferHeight = NDS_SCREEN_HEIGHT * scale;
//            bufferStride = NDS_SCREEN_WIDTH * 2 * PIXEL_SIZE * scale;
//            touchOffset = uvec2(NDS_SCREEN_WIDTH, 0);// * scale;
//            topScreenBufferOffset = 0;
//            bottomScreenBufferOffset = NDS_SCREEN_WIDTH * 2 * scale;
//
//            break;
//        case ScreenLayout::RightLeft:
//            bufferWidth = NDS_SCREEN_WIDTH * 2 * scale;
//            bufferHeight = NDS_SCREEN_HEIGHT * scale;
//            bufferStride = NDS_SCREEN_WIDTH * 2 * PIXEL_SIZE * scale;
//            touchOffset = uvec2(0, 0);
//            topScreenBufferOffset = NDS_SCREEN_WIDTH * 2 * scale;
//            bottomScreenBufferOffset = 0;
//
//            break;
//        case ScreenLayout::TopOnly:
//            bufferWidth = NDS_SCREEN_WIDTH * scale;
//            bufferHeight = NDS_SCREEN_HEIGHT * scale;
//            bufferStride = NDS_SCREEN_WIDTH * PIXEL_SIZE * scale;
//            touchOffset = uvec2(0, 0);
//            topScreenBufferOffset = 0;
//
//            break;
//        case ScreenLayout::BottomOnly:
//            bufferWidth = NDS_SCREEN_WIDTH * scale;
//            bufferHeight = NDS_SCREEN_HEIGHT * scale;
//            bufferStride = NDS_SCREEN_WIDTH * PIXEL_SIZE * scale;
//            touchOffset = uvec2(0, 0);
//            bottomScreenBufferOffset = 0;
//
//            break;
//        case ScreenLayout::HybridTop:
//        case ScreenLayout::HybridBottom:
//            bufferWidth = (NDS_SCREEN_WIDTH * hybridRatio * scale) + NDS_SCREEN_WIDTH * scale + (hybridRatio * 2);
//            bufferHeight = NDS_SCREEN_HEIGHT * hybridRatio * scale;
//            bufferStride = bufferWidth * PIXEL_SIZE;
//
//            if (Layout() == ScreenLayout::HybridTop) {
//                touchOffset = uvec2((NDS_SCREEN_WIDTH * hybridRatio) + (hybridRatio / 2), (NDS_SCREEN_HEIGHT * (hybridRatio - 1))); // * scale;
//            } else {
//                touchOffset = uvec2(0, 0);
//            }
//            break;
//    }

    // First compute the matrices for the top, bottom, and hybrid screens
//
//    // TODO: Apply the scale to the buffer dimensions later, not here

//
//    float rotation = LayoutAngle(layout);
//    vec2 scaleVector = {
//        (bufferWidth * static_cast<float>(scale)) / std::numeric_limits<uint16_t>::max(),
//        (bufferHeight * static_cast<float>(scale)) / std::numeric_limits<uint16_t>::max()
//    };
//    ivec2 translation = ivec2(touchOffset) - (ivec2(bufferWidth, 0) / 2);
//
//    // This transformation order is important!
//    touchMatrix = rotate(mat3(1), rotation) * glm::scale(mat3(1), scaleVector) * glm::translate(mat3(1), vec2(translation));
    if (!retro::set_screen_rotation(LayoutOrientation())) {
        // TODO: Rotate the screen outside screenlayout, but _before_ update;
        // if it failed, handle it accordingly in update
    }

    if (renderer == Renderer::OpenGl && this->buffer != nullptr) {
        // not needed anymore :)
        free(this->buffer);
        this->buffer = nullptr;
    } else {
        if (oldBufferLength != bufferLength || this->buffer == nullptr) {
            if (this->buffer != nullptr) free(this->buffer);
            this->buffer = (uint16_t *) malloc(bufferLength);

            memset(this->buffer, 0, bufferLength);
        }
    }

    _dirty = false;
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