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

#if defined(HAVE_TRACY) && (defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)) && !defined(__APPLE__)
#include <array>
#include <queue>

#include "PlatformOGLPrivate.h"
#include <tracy/TracyOpenGL.hpp>

namespace MelonDsDs {
    /// \brief Class for capturing OpenGL frames for Tracy.
    /// Suitable for both OpenGL renderers.
    class OpenGlTracyCapture {
    public:
        OpenGlTracyCapture(bool debug);
        ~OpenGlTracyCapture() noexcept;

        // Copying the OpenGL objects is too much of a hassle.
        OpenGlTracyCapture(const OpenGlTracyCapture&) = delete;
        OpenGlTracyCapture& operator=(const OpenGlTracyCapture&) = delete;
        OpenGlTracyCapture(OpenGlTracyCapture&&) = delete;
        OpenGlTracyCapture& operator=(OpenGlTracyCapture&&) = delete;
        void CaptureFrame(GLuint current_fbo, float scale) noexcept;
    private:
        static constexpr int FRAME_LAG = 4;
        std::array<GLuint, FRAME_LAG> _tracyTextures;
        std::array<GLuint, FRAME_LAG> _tracyFbos;
        std::array<GLuint, FRAME_LAG> _tracyPbos;
        std::array<GLsync, FRAME_LAG> _tracyFences;
        int _tracyIndex = 0;
        std::queue<int> _tracyQueue;
        bool _debug;
    };
}
#endif