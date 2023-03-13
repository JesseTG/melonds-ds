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

#ifndef MELONDS_DS_OPENGL_HPP
#define MELONDS_DS_OPENGL_HPP

namespace melonds {
    enum class CurrentRenderer
    {
        None,
        Software,
        OpenGLRenderer,
    };

    extern CurrentRenderer current_renderer;
}

namespace melonds::opengl {
    extern bool refresh_opengl;

    bool initialize();

    void deinitialize();

    /// Returns true if OpenGL is configured to be used \em and is actively being used.
    bool using_opengl();

    void render_frame(bool software);
}
#endif //MELONDS_DS_OPENGL_HPP
