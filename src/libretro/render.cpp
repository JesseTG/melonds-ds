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

#include <retro_assert.h>
#include <GPU3D.h>

#include "config.hpp"
#include "input.hpp"
#include "opengl.hpp"
#include "screenlayout.hpp"
#include "environment.hpp"

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
// TODO: Pass input state and screen layout as an argument
void melonds::render::RenderSoftware(const InputState& input_state, ScreenLayoutData& screen_layout_data) noexcept {
    retro_assert(_CurrentRenderer == Renderer::Software);
    int frontbuf = GPU::FrontBuffer;

    if (screen_layout_data.IsHybridLayout()) {
        unsigned primary = screen_layout_data.Layout() == ScreenLayout::HybridTop ? 0 : 1;

        screen_layout_data.CopyHybridScreen(GPU::Framebuffer[frontbuf][primary], HybridScreenId::Primary);

        switch (screen_layout_data.HybridSmallScreenLayout()) {
            case SmallScreenLayout::SmallScreenTop:
                screen_layout_data.CopyHybridScreen(GPU::Framebuffer[frontbuf][0], HybridScreenId::Bottom);
                break;
            case SmallScreenLayout::SmallScreenBottom:
                screen_layout_data.CopyHybridScreen(GPU::Framebuffer[frontbuf][1], HybridScreenId::Bottom);
                break;
            case SmallScreenLayout::SmallScreenDuplicate:
                screen_layout_data.CopyHybridScreen(GPU::Framebuffer[frontbuf][0], HybridScreenId::Top);
                screen_layout_data.CopyHybridScreen(GPU::Framebuffer[frontbuf][1], HybridScreenId::Bottom);
                break;
        }

        if (input_state.CursorEnabled())
            screen_layout_data.DrawCursor(input_state.TouchPosition());
    } else {
        if (screen_layout_data.TopScreenEnabled())
            screen_layout_data.CopyScreen(GPU::Framebuffer[frontbuf][0], screen_layout_data.TopScreenBufferOffset());
        if (screen_layout_data.BottomScreenEnabled())
            screen_layout_data.CopyScreen(GPU::Framebuffer[frontbuf][1],
                                          screen_layout_data.BottomScreenBufferOffset());

        if (input_state.CursorEnabled() && screen_layout_data.Layout() != ScreenLayout::TopOnly)
            screen_layout_data.DrawCursor(input_state.TouchPosition());
    }

    retro::video_refresh(
        (const uint8_t *) screen_layout_data.Buffer(),
        screen_layout_data.BufferWidth(),
        screen_layout_data.BufferHeight(),
        screen_layout_data.BufferStride()
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