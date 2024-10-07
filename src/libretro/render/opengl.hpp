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


#ifndef MELONDSDS_RENDER_OPENGL_HPP
#define MELONDSDS_RENDER_OPENGL_HPP

#include <array>
#include <memory>
#include <optional>

#include "render.hpp"

#include "PlatformOGLPrivate.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#ifdef HAVE_TRACY
#include "tracy.hpp"
#include "tracy/opengl.hpp"
#endif

namespace MelonDsDs {
    using glm::vec2;
    using glm::vec4;

    class OpenGLRenderState final : public RenderState {
    public:
        static std::unique_ptr<OpenGLRenderState> New() noexcept;
        OpenGLRenderState();
        ~OpenGLRenderState() noexcept override;
        OpenGLRenderState(const OpenGLRenderState&) = delete;
        OpenGLRenderState(OpenGLRenderState&&) = delete;
        OpenGLRenderState& operator=(const OpenGLRenderState&) = delete;
        OpenGLRenderState& operator=(OpenGLRenderState&&) = delete;
        [[nodiscard]] bool Ready() const noexcept override { return _contextInitialized; }
        void Render(
            melonDS::NDS& nds,
            const InputState& input,
            const CoreConfig& config,
            const ScreenLayoutData& screenLayout
        ) noexcept override;
        // Requests that the OpenGL context be refreshed.
        void RequestRefresh() noexcept override {
            _needsRefresh = true;
        }

        void ContextReset(melonDS::NDS& nds, const CoreConfig& config);
        void ContextDestroyed();
    private:
        struct Vertex {
            vec2 position;
            vec2 texcoord;
        };

        static_assert(sizeof(Vertex) == sizeof(vec2::value_type) * 4);

        void SetUpCoreOpenGlState(const CoreConfig& config);
        void InitFrameState(melonDS::NDS& nds, const CoreConfig& config, const ScreenLayoutData& screenLayout) noexcept;
        void InitVertices(const ScreenLayoutData& screenLayout) noexcept;
        bool _openGlDebugAvailable = false;
        bool _needsRefresh = true;
        bool _contextInitialized = false;
        GLuint _screenProgram = 0;
        GLuint screen_framebuffer_texture = 0;
        std::array<Vertex, 18> screen_vertices {};
        unsigned vertexCount = 0;
        GLuint vao = 0;
        GLuint vbo = 0;

        struct {
            vec2 uScreenSize;
            uint32_t u3DScale;
            uint32_t uFilterMode;
            vec4 cursorPos;
            bool cursorVisible;
        } GL_ShaderConfig {};

        GLuint ubo = 0;

#ifdef HAVE_TRACY
        std::optional<OpenGlTracyCapture> _tracyCapture;
#endif
    };
}

#endif // MELONDSDS_RENDER_OPENGL_HPP
