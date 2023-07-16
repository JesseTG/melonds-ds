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
    buffer(nullptr) {
}

melonds::ScreenLayoutData::~ScreenLayoutData() {
    delete[] buffer;
}

void melonds::ScreenLayoutData::CopyScreen(const uint32_t* src, unsigned offset) noexcept {
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
            uint32_t *offset = base_offset + (y * bufferSize.x) + x;
            uint32_t pixel = *offset;
            *(uint32_t *) offset = (0xFFFFFF - pixel) | 0xFF000000;
        }
    }
}


void melonds::ScreenLayoutData::CombineScreens(const uint32_t* topBuffer, const uint32_t* bottomBuffer, std::optional<glm::ivec2> touch) noexcept {
    if (!buffer)
        return;

    switch (Layout()) {
        case ScreenLayout::HybridBottom:
        case ScreenLayout::HybridTop:
            // TODO: Implement
        default:
            break;
    }
}

using melonds::ScreenLayoutData;

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


size_t melonds::ScreenLayoutData::GetTopScreenBufferOffset(unsigned scale) const noexcept {
    switch (Layout()) {
        case ScreenLayout::RightLeft:
            return NDS_SCREEN_WIDTH * 2 * scale;
        case ScreenLayout::BottomTop:
            return bufferSize.x * (NDS_SCREEN_HEIGHT + screenGap) * scale;
        default:
            return 0;
    }
}
size_t melonds::ScreenLayoutData::GetBottomScreenBufferOffset(unsigned int scale) const noexcept {
    switch (Layout()) {
        case ScreenLayout::LeftRight:
            return NDS_SCREEN_WIDTH * 2 * scale;
        case ScreenLayout::TopBottom:
            return bufferSize.x * (NDS_SCREEN_HEIGHT + screenGap) * scale;
        default:
            return 0;
    }
}

size_t melonds::ScreenLayoutData::GetHybridScreenBufferOffset(unsigned scale) const noexcept {
    return 0;
    // TODO: When I add support for right-ended hybrid layouts, implement this
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
    bottomScreenMatrixInverse = inverse(bottomScreenMatrix);

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

    topScreenBufferOffset = GetTopScreenBufferOffset(scale);
    bottomScreenBufferOffset = GetBottomScreenBufferOffset(scale);
    hybridScreenBufferOffset = GetHybridScreenBufferOffset(scale);
    pointerMatrix = math::ts<float>(vec2(bufferSize) / 2.0f, vec2(bufferSize) / (2.0f * RETRO_MAX_POINTER_COORDINATE<float>));

    if (!retro::set_screen_rotation(LayoutOrientation())) {
        // TODO: Rotate the screen outside screenlayout, but _before_ update;
        // if it failed, handle it accordingly in update
    }

    if (renderer == Renderer::OpenGl && buffer != nullptr) {
        // not needed anymore :)
        delete[] buffer;
        buffer = nullptr;
    } else {
        if (bufferSize != oldBufferSize || buffer == nullptr) {
            delete[] buffer;
            buffer = new uint32_t[bufferSize.x * bufferSize.y];

            memset(buffer, 0, bufferSize.x * bufferSize.y * sizeof(uint32_t));
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