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

#include "render.hpp"

#include <optional>

#include <glm/vec2.hpp>
#include <retro_assert.h>
#include <GPU3D.h>

#include "config.hpp"
#include "input.hpp"
#include "opengl.hpp"
#include "screenlayout.hpp"
#include "environment.hpp"

using std::nullopt;
using std::optional;
using glm::ivec2;
using glm::uvec2;

namespace melonds::render {
    static Renderer _CurrentRenderer = Renderer::None;
    static void RenderSoftware(const InputState& input_state, ScreenLayoutData& screenLayout) noexcept;
}

void melonds::render::Initialize(Renderer renderer) {
    using retro::log;

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    // Initialize the opengl state if needed
    switch (renderer) {
        // Depending on which renderer we want to use...
        case Renderer::OpenGl:
            if (melonds::opengl::Initialize()) {
                _CurrentRenderer = Renderer::OpenGl;
                log(RETRO_LOG_DEBUG, "Requested OpenGL context");
            } else {
                _CurrentRenderer = Renderer::Software;
                log(RETRO_LOG_ERROR, "Failed to initialize OpenGL renderer, falling back to software rendering");
                // TODO: Display a message stating that we're falling back to software rendering
            }
            break;
        default:
            log(RETRO_LOG_WARN, "Unknown renderer %d, falling back to software rendering", static_cast<int>(renderer));
            // Intentional fall-through
        case Renderer::Software:
            _CurrentRenderer = Renderer::Software;
            log(RETRO_LOG_INFO, "Using software renderer");
            break;
    }
#else
    _CurrentRenderer = Renderer::Software;
    log(RETRO_LOG_INFO, "OpenGL is not supported by this build, using software renderer");
#endif
}

bool melonds::render::ReadyToRender() noexcept {
    using melonds::Renderer;
    if (GPU3D::CurrentRenderer == nullptr) {
        // If the emulator doesn't yet have an assigned renderer...
        return false;
    }

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (_CurrentRenderer == Renderer::OpenGl && !melonds::opengl::ContextInitialized()) {
        // If we're using OpenGL, but it isn't ready...
        return false;
    }
#endif

    // Software rendering doesn't need us to set up any context, the frontend does that
    return true;
}

// TODO: Consider using RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER
void melonds::render::RenderSoftware(const InputState& input_state, ScreenLayoutData& screen_layout_data) noexcept {
    retro_assert(_CurrentRenderer == Renderer::Software);
    int frontbuf = GPU::FrontBuffer;

    const uint32_t* topScreenBuffer = GPU::Framebuffer[GPU::FrontBuffer][0];
    const uint32_t* bottomScreenBuffer = GPU::Framebuffer[GPU::FrontBuffer][1];
    optional<ivec2> touch = input_state.CursorEnabled() ? optional<ivec2>(input_state.TouchPosition()) : nullopt;
    //screen_layout_data.CombineScreens(topScreenBuffer, bottomScreenBuffer, touch);
    screen_layout_data.Clear();
    ScreenLayout layout = screen_layout_data.Layout();
    if (IsHybridLayout(layout)) {
        unsigned primary = layout == ScreenLayout::HybridTop ? 0 : 1;

        screen_layout_data.CopyHybridScreen(GPU::Framebuffer[frontbuf][primary], HybridScreenId::Primary, screen_layout_data.HybridScreenTranslation());

        HybridSideScreenDisplay smallScreenLayout = screen_layout_data.HybridSmallScreenLayout();

        if (smallScreenLayout == HybridSideScreenDisplay::Both || layout == ScreenLayout::HybridBottom) {
            // If we should display both screens, or if the bottom one is the primary...
            screen_layout_data.CopyHybridScreen(GPU::Framebuffer[frontbuf][0], HybridScreenId::Top, screen_layout_data.TopScreenTranslation());
        }

        if (smallScreenLayout == HybridSideScreenDisplay::Both || layout == ScreenLayout::HybridTop) {
            // If we should display both screens, or if the top one is being focused...
            screen_layout_data.CopyHybridScreen(GPU::Framebuffer[frontbuf][1], HybridScreenId::Bottom, screen_layout_data.BottomScreenTranslation());
        }

        if (input_state.CursorEnabled()) {
            screen_layout_data.DrawCursor(input_state.TouchPosition(), screen_layout_data.BottomScreenMatrix());
            screen_layout_data.DrawCursor(input_state.TouchPosition(), screen_layout_data.HybridScreenMatrix());
        }

    } else {
        if (layout != ScreenLayout::BottomOnly)
            screen_layout_data.CopyScreen(GPU::Framebuffer[frontbuf][0], screen_layout_data.TopScreenTranslation());

        if (layout != ScreenLayout::TopOnly)
            screen_layout_data.CopyScreen(GPU::Framebuffer[frontbuf][1], screen_layout_data.BottomScreenTranslation());

        if (input_state.CursorEnabled() && layout != ScreenLayout::TopOnly)
            screen_layout_data.DrawCursor(input_state.TouchPosition(), screen_layout_data.BottomScreenMatrix());
    }

    retro::video_refresh(
        (const uint8_t *) screen_layout_data.Buffer()[0],
        screen_layout_data.Buffer().Width(),
        screen_layout_data.Buffer().Height(),
        screen_layout_data.Buffer().Stride()
    );
}

melonds::Renderer melonds::render::CurrentRenderer() noexcept {
    return _CurrentRenderer;
}

void melonds::render::Render(const InputState& input_state, ScreenLayoutData& screenLayout) noexcept {
    switch (_CurrentRenderer) {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
        case Renderer::OpenGl:
            melonds::opengl::Render(input_state, screenLayout);
            break;
#endif
        case Renderer::Software:
        default:
            render::RenderSoftware(input_state, screenLayout);
            break;
    }
}