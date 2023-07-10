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
    class InputState;
    class ScreenLayoutData;
}

namespace melonds::opengl {
    // Requests that the OpenGL context be refreshed.
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    void RequestOpenGlRefresh();
#else
    inline void RequestOpenGlRefresh() {}
#endif

    bool Initialize();

    void deinitialize();

    void Render(const InputState& state, const ScreenLayoutData& screenLayout) noexcept;

    bool ContextInitialized();
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    bool UsingOpenGl();
#else
    inline bool UsingOpenGl() { return false; }
#endif
}
#endif //MELONDS_DS_OPENGL_HPP
