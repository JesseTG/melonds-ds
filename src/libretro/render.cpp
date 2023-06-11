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

#include <libretro.h>
#include <glsm/glsm.h>
#include <glsm/glsmsym.h>

#include <GPU3D.h>
#include <frontend/qt_sdl/Config.h>

#include "config.hpp"
#include "input.hpp"
#include "opengl.hpp"
#include "screenlayout.hpp"
#include "environment.hpp"

bool melonds::render::ReadyToRender() {
    using melonds::Renderer;
    if (GPU3D::CurrentRenderer == nullptr) {
        // If the emulator doesn't yet have an assigned renderer...
        return false;
    }

    if (Config::Retro::CurrentRenderer == Renderer::OpenGl && !melonds::opengl::ContextInitialized()) {
        // If we're using OpenGL, but it isn't ready...
        return false;
    }

    // Software rendering doesn't need us to set up any context, the frontend does that
    return true;
}

// TODO: Consider using RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER
// TODO: Pass input state and screen layout as an argument
void melonds::render::RenderSoftware() {
    int frontbuf = GPU::FrontBuffer;

    if (screen_layout_data.hybrid) {
        unsigned primary = screen_layout_data.displayed_layout == ScreenLayout::HybridTop ? 0 : 1;

        screen_layout_data.copy_hybrid_screen(GPU::Framebuffer[frontbuf][primary], ScreenId::Primary);

        switch (screen_layout_data.hybrid_small_screen) {
            case SmallScreenLayout::SmallScreenTop:
                screen_layout_data.copy_hybrid_screen(GPU::Framebuffer[frontbuf][0], ScreenId::Bottom);
                break;
            case SmallScreenLayout::SmallScreenBottom:
                screen_layout_data.copy_hybrid_screen(GPU::Framebuffer[frontbuf][1], ScreenId::Bottom);
                break;
            case SmallScreenLayout::SmallScreenDuplicate:
                screen_layout_data.copy_hybrid_screen(GPU::Framebuffer[frontbuf][0], ScreenId::Top);
                screen_layout_data.copy_hybrid_screen(GPU::Framebuffer[frontbuf][1], ScreenId::Bottom);
                break;
        }

        if (input_state.cursor_enabled())
            screen_layout_data.draw_cursor(input_state.touch_x, input_state.touch_y);
    } else {
        if (screen_layout_data.enable_top_screen)
            screen_layout_data.copy_screen(GPU::Framebuffer[frontbuf][0], screen_layout_data.top_screen_offset);
        if (screen_layout_data.enable_bottom_screen)
            screen_layout_data.copy_screen(GPU::Framebuffer[frontbuf][1],
                                           screen_layout_data.bottom_screen_offset);

        if (input_state.cursor_enabled() && current_screen_layout() != ScreenLayout::TopOnly)
            screen_layout_data.draw_cursor(input_state.touch_x, input_state.touch_y);
    }

    retro::video_refresh(
        (uint8_t *) screen_layout_data.buffer_ptr,
        screen_layout_data.buffer_width,
        screen_layout_data.buffer_height,
        screen_layout_data.buffer_width * sizeof(uint32_t)
    );
}