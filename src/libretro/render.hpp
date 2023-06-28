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

#ifndef MELONDS_DS_RENDER_HPP
#define MELONDS_DS_RENDER_HPP

#include "input.hpp"

namespace melonds::render {
    /// Returns true if all global state necessary for rendering is ready.
    /// This includes the OpenGL context (if applicable) and the emulator's renderer.
    bool ReadyToRender();

    /// Renders a frame with software rendering and submits it to libretro for display.
    void RenderSoftware(const InputState& input_state);
}

#endif //MELONDS_DS_RENDER_HPP
