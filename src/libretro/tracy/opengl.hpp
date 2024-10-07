/*
    Copyright 2024 Jesse Talavera

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

#pragma once

#if defined(HAVE_TRACY) && (defined(HAVE_OPENGL) || defined(HAVE_OPENGLES))
#include <array>
#include <queue>

#include "PlatformOGLPrivate.h"
#include <tracy/TracyOpenGL.hpp>

namespace MelonDsDs {
    /// \brief Class for capturing OpenGL frames for Tracy.
    /// Suitable for both OpenGL renderers.
    class OpenGlTracyCapture {
    public:
        OpenGlTracyCapture();
        ~OpenGlTracyCapture() noexcept;

        // Copying the OpenGL objects is too much of a hassle.
        OpenGlTracyCapture(const OpenGlTracyCapture&) = delete;
        OpenGlTracyCapture& operator=(const OpenGlTracyCapture&) = delete;
        OpenGlTracyCapture(OpenGlTracyCapture&&) = delete;
        OpenGlTracyCapture& operator=(OpenGlTracyCapture&&) = delete;
        void CaptureFrame(float scale) noexcept;
    private:
        std::array<GLuint, 4> _tracyTextures;
        std::array<GLuint, 4> _tracyFbos;
        std::array<GLuint, 4> _tracyPbos;
        std::array<GLsync, 4> _tracyFences;
        int _tracyIndex = 0;
        std::queue<int> _tracyQueue;
    };
}
#endif