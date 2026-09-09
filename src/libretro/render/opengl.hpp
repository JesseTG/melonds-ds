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
#include <utility>

#include <libretro.h>

#include "render.hpp"

#include "PlatformOGLPrivate.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#if defined(HAVE_TRACY) && !defined(__APPLE__)
#include "tracy/opengl.hpp"
#endif

namespace MelonDsDs {
    using glm::vec2;
    using glm::vec3;
    using glm::vec4;

    class OpenGLRenderState final : public RenderState {
    public:
        /// Returns nullptr if the frontend won't provide the OpenGL context \c mode needs.
        static std::unique_ptr<OpenGLRenderState> New(RenderMode mode) noexcept;
        /// \param mode Which OpenGL render mode this state is for; must satisfy \c UsesOpenGl.
        explicit OpenGLRenderState(RenderMode mode);
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

        ShaderCompileProgress CompileShaders(melonDS::NDS& nds, std::chrono::microseconds budget) noexcept override;

        [[nodiscard]] RenderMode Mode() const noexcept { return _mode; }
        [[nodiscard]] bool UsesComputeRenderer() const noexcept { return _mode == RenderMode::Compute; }

        /// True if the context we've been given can drive \c mode,
        /// whichever mode we originally asked the frontend for.
        /// Frontends routinely hand out a newer context than the core requested
        /// (Mesa and RetroArch's WGL and GLX drivers all return the driver's highest version),
        /// so a context obtained for one OpenGL renderer will usually run the other one too.
        /// Always false until the context has been reset, since that's when its version is known.
        [[nodiscard]] bool CanHost(RenderMode mode) const noexcept;

        /// Switches to another OpenGL renderer without asking the frontend for a new context.
        /// Only valid for a mode that \c CanHost accepts;
        /// \c RenderStateWrapper::UpdateRenderer installs the new renderer afterwards.
        void SetMode(RenderMode mode) noexcept;

        /// Which renderer melonDS is using on this context,
        /// or \c nullopt if we haven't installed one (or if the context went away).
        [[nodiscard]] std::optional<RenderMode> InstalledMode() const noexcept { return _installedMode; }

        /// Records that melonDS is now rendering with \c Mode().
        void MarkRendererInstalled() noexcept { _installedMode = _mode; }

        void ContextReset(melonDS::NDS& nds, const CoreConfig& config);
        void ContextDestroyed();
    private:
        struct Vertex {
            vec2 position;
            /// The third coordinate selects one of the two screens
            /// within melonDS's output texture, which is a 2D array texture.
            vec3 texcoord;
        };

        static_assert(sizeof(Vertex) == sizeof(vec2::value_type) * 5);

        void SetUpCoreOpenGlState(const CoreConfig& config);
        // Records the context's version in _contextVersion,
        // then throws opengl_not_initialized_exception if it's too old for _mode.
        void CheckContextVersion();
        void InitFrameState(melonDS::NDS& nds, const CoreConfig& config, const ScreenLayoutData& screenLayout) noexcept;
        void InitVertices(const ScreenLayoutData& screenLayout) noexcept;

        // The frontend's framebuffer, i.e. the one we draw the final image into.
        // Only valid after the OpenGL context has been reset.
        [[nodiscard]] GLuint CurrentFramebuffer() const noexcept;

        // Applies the OpenGL state that melonDS DS's screen blit relies on,
        // and binds this object's OpenGL resources.
        // Call before making any OpenGL calls on behalf of the core;
        // the frontend may have changed any of this state in the meantime.
        void BindState() noexcept;

        // Resets all OpenGL state that melonDS DS (including melonDS's own renderer) touches
        // back to OpenGL's defaults, so the frontend can't be tripped up by what we left behind.
        // Call after the core is done making OpenGL calls for the frame.
        void UnbindState() noexcept;

        // Which of melonDS's OpenGL-based 3D renderers this state drives.
        // Decides the version we ask the frontend for,
        // but it can change afterwards if the context we got can drive the other renderer too.
        RenderMode _mode;
        // Which renderer melonDS is using, so we know when _mode has moved on without it.
        std::optional<RenderMode> _installedMode = std::nullopt;
        // The context's actual version, which is at least the one we asked for.
        // Only meaningful once the context has been reset.
        std::pair<GLint, GLint> _contextVersion {0, 0};
        bool _openGlDebugAvailable = false;
        bool _needsRefresh = true;
        bool _contextInitialized = false;
        // melonDS's top-level renderer doesn't expose its current settings,
        // so we track the ones we last gave it to know when they've changed.
        unsigned _appliedScaleFactor = 0;
        bool _appliedBetterPolygons = false;
        bool _appliedHiresCoordinates = false;
        GLuint _screenProgram = 0;
        std::array<Vertex, 18> screen_vertices {};
        unsigned vertexCount = 0;
        // What we ask of the frontend's OpenGL context,
        // plus the callbacks it gives us in return.
        // Set up in the constructor; the frontend fills in the function pointers.
        retro_hw_render_callback _hw_render {};
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

#if defined(HAVE_TRACY) && !defined(__APPLE__)
        std::optional<OpenGlTracyCapture> _tracyCapture;
#endif
    };
}

#endif // MELONDSDS_RENDER_OPENGL_HPP
