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
#include "opengl.hpp"

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