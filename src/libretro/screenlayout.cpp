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

#include <array>
#include <cmath>

#include <GPU3D.h>

#include <glm/gtx/matrix_transform_2d.hpp>
#include <retro_assert.h>

#include "config/config.hpp"
#include "math.hpp"
#include "tracy.hpp"
#include "render/render.hpp"

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

MelonDsDs::ScreenLayoutData::ScreenLayoutData() :
    _dirty(true), // Uninitialized
    orientation(retro::ScreenOrientation::Normal),
    joystickMatrix(1), // Identity matrix
    topScreenMatrix(1),
    bottomScreenMatrix(1),
    bottomScreenMatrixInverse(1),
    hybridScreenMatrix(1),
    pointerMatrix(1),
    hybridRatio(2),
    _numberOfLayouts(1) {
}

MelonDsDs::ScreenLayoutData::~ScreenLayoutData() noexcept {
}

/// For a screen in the top left corner
mat3 NorthwestMatrix(unsigned resolutionScale) noexcept {
    return scale(mat3(1), vec2(resolutionScale));
}

/// For a screen on the bottom that accounts for the screen gap
constexpr mat3 SouthwestMatrix(unsigned resolutionScale, unsigned screenGap) noexcept {
    using namespace MelonDsDs;
    return math::ts<float>(
        vec2(0, resolutionScale * (NDS_SCREEN_HEIGHT + screenGap)),
        vec2(resolutionScale)
    );
}

/// For a screen on the right
constexpr mat3 EastMatrix(unsigned resolutionScale) noexcept {
    using namespace MelonDsDs;
    return math::ts<float>(
        vec2(resolutionScale * NDS_SCREEN_WIDTH, 0),
        vec2(resolutionScale)
    );
}

/// For the northeast hybrid screen
constexpr mat3 HybridWestMatrix(unsigned resolutionScale, unsigned hybridRatio) noexcept {
    using namespace MelonDsDs;
    return math::ts<float>(
        vec2(0),
        vec2(resolutionScale * hybridRatio)
    );
}

/// For the northeast hybrid screen
constexpr mat3 HybridNortheastMatrix(unsigned resolutionScale, unsigned hybridRatio) noexcept {
    using namespace MelonDsDs;
    return math::ts<float>(
        vec2(resolutionScale * hybridRatio * NDS_SCREEN_WIDTH, 0),
        vec2(resolutionScale)
    );
}

/// For the southeast hybrid screen
constexpr mat3 HybridSoutheastMatrix(unsigned resolutionScale, unsigned hybridRatio) noexcept {
    using namespace MelonDsDs;
    return math::ts<float>(
        vec2(resolutionScale * hybridRatio * NDS_SCREEN_WIDTH, resolutionScale * NDS_SCREEN_HEIGHT * (hybridRatio - 1)),
        vec2(resolutionScale)
    );
}

mat3 MelonDsDs::ScreenLayoutData::GetTopScreenMatrix(unsigned scale) const noexcept {
    ZoneScopedN("MelonDsDs::ScreenLayoutData::GetTopScreenMatrix");
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

mat3 MelonDsDs::ScreenLayoutData::GetBottomScreenMatrix(unsigned scale) const noexcept {
    ZoneScopedN("MelonDsDs::ScreenLayoutData::GetBottomScreenMatrix");
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

glm::mat3 MelonDsDs::ScreenLayoutData::GetHybridScreenMatrix(unsigned scale) const noexcept {
    ZoneScopedN(TracyFunction);
    switch (Layout()) {
        case ScreenLayout::HybridBottom:
        case ScreenLayout::HybridTop:
            return HybridWestMatrix(scale, hybridRatio);
        default:
            return mat3(1);
    }
}

void MelonDsDs::ScreenLayoutData::Apply(const CoreConfig& config, const RenderStateWrapper& renderState) noexcept {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    SetScale(renderState.GetRenderer() == Renderer::Software ? 1 : config.ScaleFactor());
#else
    SetScale(1);
#endif
    SetLayouts(config.ScreenLayouts());
    HybridSmallScreenLayout(config.SmallScreenLayout());
    ScreenGap(config.ScreenGap());
    HybridRatio(config.HybridRatio());
    Update();
}

void MelonDsDs::ScreenLayoutData::Update() noexcept {
    ZoneScopedN(TracyFunction);

    // These points represent the NDS screen coordinates without transformations
    constexpr array<vec2, 4> baseScreenPoints = {
        vec2(0, 0), // northwest
        vec2(NDS_SCREEN_WIDTH, 0), // northeast
        vec2(NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT), // southeast
        vec2(0, NDS_SCREEN_HEIGHT) // southwest
    };

    // Get the matrices we'll be using
    // (except the pointer matrix, we need to compute the buffer size first)
    topScreenMatrix = GetTopScreenMatrix(resolutionScale);
    bottomScreenMatrix = GetBottomScreenMatrix(resolutionScale);
    hybridScreenMatrix = GetHybridScreenMatrix(resolutionScale);
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
    retro::ScreenOrientation newOrientation = LayoutOrientation(layout);

    if (retro::set_screen_rotation(newOrientation)) {
        // Try to rotate the screen. If that failed...
        pointerMatrix = glm::rotate(pointerMatrix, LayoutAngle(layout));
        orientation = newOrientation;
    } else if (newOrientation != retro::ScreenOrientation::Normal) {
        // A rotation to normal orientation may "fail", even though it's the default.
        // So only log an error if we're trying to rotate to something besides 0 degrees.
        retro::set_error_message("Failed to rotate screen.");
    }

    _dirty = false;
}

retro_game_geometry MelonDsDs::ScreenLayoutData::Geometry(const melonDS::Renderer3D& renderer) const noexcept {
    retro_game_geometry geometry {
        .base_width = BufferWidth(),
        .base_height = BufferHeight(),
        .max_width = MaxSoftwareRenderedWidth(),
        .max_height = MaxSoftwareRenderedHeight(),
        .aspect_ratio = BufferAspectRatio(),
    };

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (renderer.Accelerated) {
        geometry.max_width = MaxOpenGlRenderedWidth();
        geometry.max_height = MaxOpenGlRenderedHeight();
    }
#endif
    static_assert(MaxSoftwareRenderedWidth() > 0);
    static_assert(MaxSoftwareRenderedHeight() > 0);
    retro_assert(geometry.base_width > 0);
    retro_assert(geometry.base_height > 0);
    retro_assert(geometry.max_height >= geometry.base_height);
    retro_assert(geometry.max_width >= geometry.base_width);
    retro_assert(std::isfinite(geometry.aspect_ratio));

    return geometry;
}
