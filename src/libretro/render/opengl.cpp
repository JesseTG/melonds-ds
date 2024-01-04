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


#include "opengl.hpp"

#include <array>

#include <GPU3D_OpenGL.h>
#include <NDS.h>

#include <gfx/gl_capabilities.h>
#include <glsm/glsm.h>
#include <retro_assert.h>
#include <embedded/melondsds_fragment_shader.h>
#include <embedded/melondsds_vertex_shader.h>

#include "../core/core.hpp"
#include "exceptions.hpp"
#include "screenlayout.hpp"
#include "tracy.hpp"

using glm::ivec2;
using glm::vec2;
using std::array;
using MelonDsDs::ScreenLayout;

constexpr float PIXEL_PAD = 1.0f / (MelonDsDs::NDS_SCREEN_HEIGHT * 2 + 2);
constexpr unsigned VERTEXES_PER_SCREEN = 6;
constexpr array TOP_SCREEN_TEXCOORDS {
    vec2(0), // northwest
    vec2(0, 0.5f - PIXEL_PAD), // southwest
    vec2(1, 0.5f - PIXEL_PAD), // southeast
    vec2(0), //northwest
    vec2(1, 0), // northeast
    vec2(1, 0.5f - PIXEL_PAD), // southeast
};
constexpr array BOTTOM_SCREEN_TEXCOORDS {
    vec2(0, 0.5f + PIXEL_PAD), // northwest
    vec2(0, 1), // southwest
    vec2(1), // southeast
    vec2(0, 0.5f + PIXEL_PAD), // northwest
    vec2(1, 0.5f + PIXEL_PAD), // northeast
    vec2(1), // southeast
};

constexpr array<unsigned, 18> GetPositionIndexes(MelonDsDs::ScreenLayout layout) noexcept {
    array<unsigned, VERTEXES_PER_SCREEN> topPositionIndexes = {0, 3, 2, 0, 1, 2};
    array<unsigned, VERTEXES_PER_SCREEN> bottomPositionIndexes = {4, 7, 6, 4, 5, 6};
    array<unsigned, VERTEXES_PER_SCREEN> hybridPositionIndexes = {8, 11, 10, 8, 9, 10};
    array<unsigned, VERTEXES_PER_SCREEN*3> indexes = {};

    switch (layout) {
        case ScreenLayout::TopBottom:
        case ScreenLayout::TurnLeft:
        case ScreenLayout::TurnRight:
        case ScreenLayout::UpsideDown:
        case ScreenLayout::LeftRight:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                indexes[i] = topPositionIndexes[i];
                indexes[i + VERTEXES_PER_SCREEN] = bottomPositionIndexes[i];
            }
            break;
        case ScreenLayout::RightLeft:
        case ScreenLayout::BottomTop:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                indexes[i] = bottomPositionIndexes[i];
                indexes[i + VERTEXES_PER_SCREEN] = topPositionIndexes[i];
            }
            break;
        case ScreenLayout::TopOnly:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                indexes[i] = topPositionIndexes[i];
            }
            break;
        case ScreenLayout::BottomOnly:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                indexes[i] = bottomPositionIndexes[i];
            }
            break;
        case ScreenLayout::HybridTop:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                indexes[i] = hybridPositionIndexes[i];
                indexes[i + VERTEXES_PER_SCREEN] = bottomPositionIndexes[i];
                indexes[i + VERTEXES_PER_SCREEN*2] = topPositionIndexes[i];
            }
            break;
        case ScreenLayout::HybridBottom:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                indexes[i] = hybridPositionIndexes[i];
                indexes[i + VERTEXES_PER_SCREEN] = topPositionIndexes[i];
                indexes[i + VERTEXES_PER_SCREEN*2] = bottomPositionIndexes[i];
            }
            break;
    }

    return indexes;
}

constexpr unsigned GetVertexCount(ScreenLayout layout, MelonDsDs::HybridSideScreenDisplay hybridScreen) noexcept {
    switch (layout) {
        case ScreenLayout::TopOnly:
        case ScreenLayout::BottomOnly:
            return 6; // 1 screen, 2 triangles
        case ScreenLayout::HybridTop:
        case ScreenLayout::HybridBottom:
            if (hybridScreen == MelonDsDs::HybridSideScreenDisplay::Both)
                return 18; // 3 screens, 6 triangles
        [[fallthrough]];
        default:
            return 12; // 2 screens, 4 triangles
    }
}

// HACK: Defined in glsm.c, but we need to peek into it occasionally
extern retro_hw_render_callback hw_render;

static const char* const SHADER_PROGRAM_NAME = "melonDS DS Shader Program";


std::unique_ptr<MelonDsDs::OpenGLRenderState> MelonDsDs::OpenGLRenderState::New() noexcept {
    ZoneScopedN(TracyFunction);
    try {
        return std::make_unique<OpenGLRenderState>();
    } catch (const opengl_not_initialized_exception& e) {
        retro::debug("OpenGL context could not be initialized: %s", e.what());
        return nullptr;
    }
}

MelonDsDs::OpenGLRenderState::OpenGLRenderState() {
    ZoneScopedN(TracyFunction);
    retro::debug(TracyFunction);
    glsm_ctx_params_t params = {};

    // MelonDS DS wants an opengl 3.1 context, so glcore is required for mesa compatibility
    params.context_type = RETRO_HW_CONTEXT_OPENGL;
    params.major = 3;
    params.minor = 1;
    params.context_reset = HardwareContextReset;
    params.context_destroy = HardwareContextDestroyed;
    params.environ_cb = retro::environment;
    params.stencil = false;
    params.framebuffer_lock = nullptr;

#ifndef NDEBUG
    hw_render.debug_context = true;
#endif

    if (!glsm_ctl(GLSM_CTL_STATE_CONTEXT_INIT, &params)) {
        throw opengl_not_initialized_exception();
    }

#ifndef NDEBUG
    retro_assert(hw_render.debug_context);
#endif

    gl_query_core_context_set(hw_render.context_type == RETRO_HW_CONTEXT_OPENGL_CORE);
}

MelonDsDs::OpenGLRenderState::~OpenGLRenderState() noexcept {
    retro::debug(TracyFunction);
    if (_contextInitialized) {
        TracyGpuZone(TracyFunction);
        glsm_ctl(GLSM_CTL_STATE_BIND, nullptr);
        glDeleteTextures(1, &screen_framebuffer_texture);

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);

        melonDS::OpenGL::DeleteShaderProgram(shader.data());
        glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr);
    }
    glsm_ctl(GLSM_CTL_STATE_CONTEXT_DESTROY, nullptr);
    gl_query_core_context_unset();
}

void MelonDsDs::OpenGLRenderState::ContextReset(melonDS::NDS& nds, const CoreConfig& config) {
    ZoneScopedN(TracyFunction);
    retro::debug(TracyFunction);

    // Initialize all OpenGL function pointers
    glsm_ctl(GLSM_CTL_STATE_CONTEXT_RESET, nullptr);
    TracyGpuContext; // Must be called AFTER the function pointers are bound!

    // Initialize global OpenGL resources (e.g. VAOs) and get config info (e.g. limits)
    glsm_ctl(GLSM_CTL_STATE_SETUP, nullptr);

    // Start using global OpenGL structures
    {
        TracyGpuZone("GLSM_CTL_STATE_BIND");
        glsm_ctl(GLSM_CTL_STATE_BIND, nullptr);
    }

    // HACK: Makes the core resilient to context loss by cleaning up the stale OpenGL renderer
    // (The "correct" way to do this would be to add a Reinitialize() method to GLRenderer
    // that recreates all resources)
    nds.GPU.GPU3D.SetCurrentRenderer(std::make_unique<melonDS::SoftRenderer>());
    auto renderer = melonDS::GLRenderer::New();
    if (!renderer) {
        throw opengl_not_initialized_exception();
    }
    renderer->SetRenderSettings(config.BetterPolygonSplitting(), config.ScaleFactor());
    nds.GPU.SetRenderer3D(std::move(renderer));

    SetUpCoreOpenGlState(config);
    _contextInitialized = true;

    // Stop using OpenGL structures
    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr); // Always succeeds

    retro::debug("OpenGL context reset successfully.");
}

// Sets up OpenGL resources specific to melonDS
void MelonDsDs::OpenGLRenderState::SetUpCoreOpenGlState(const CoreConfig& config) {
    ZoneScopedN(TracyFunction);
    TracyGpuZone(TracyFunction);
    retro::debug(TracyFunction);

    {
        ZoneScopedN("gl_check_capability");
        _openGlDebugAvailable = gl_check_capability(GL_CAPS_DEBUG);
    }
    if (_openGlDebugAvailable) {
        retro::debug("OpenGL debugging extensions are available");
    }

    if (!melonDS::OpenGL::BuildShaderProgram(embedded_melondsds_vertex_shader, embedded_melondsds_fragment_shader, shader.data(), SHADER_PROGRAM_NAME))
        throw shader_compilation_failed_exception("Failed to compile melonDS DS shaders.");

    if (_openGlDebugAvailable) {
        glObjectLabel(GL_SHADER, shader[0], -1, "melonDS DS Vertex Shader");
        glObjectLabel(GL_SHADER, shader[1], -1, "melonDS DS Fragment Shader");
        glObjectLabel(GL_PROGRAM, shader[2], -1, SHADER_PROGRAM_NAME);
    }

    glBindAttribLocation(shader[2], 0, "vPosition");
    glBindAttribLocation(shader[2], 1, "vTexcoord");
    glBindFragDataLocation(shader[2], 0, "oColor");

    if (!melonDS::OpenGL::LinkShaderProgram(shader.data()))
        throw shader_compilation_failed_exception("Failed to link compiled shaders.");

    GLuint uConfigBlockIndex = glGetUniformBlockIndex(shader[2], "uConfig");
    glUniformBlockBinding(shader[2], uConfigBlockIndex, 16); // TODO: Where does 16 come from? It's not a size.

    glUseProgram(shader[2]);
    GLuint uni_id = glGetUniformLocation(shader[2], "ScreenTex");
    glUniform1i(uni_id, 0);

    memset(&GL_ShaderConfig, 0, sizeof(GL_ShaderConfig));

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    if (_openGlDebugAvailable) {
        glObjectLabel(GL_BUFFER, ubo, -1, "melonDS DS Shader Config UBO");
    }
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GL_ShaderConfig), &GL_ShaderConfig, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 16, ubo);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (_openGlDebugAvailable) {
        glObjectLabel(GL_BUFFER, vbo, -1, "melonDS DS Screen Vertex Buffer");
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), nullptr, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    if (_openGlDebugAvailable) {
        glObjectLabel(GL_VERTEX_ARRAY, vao, -1, "melonDS DS Screen VAO");
    }
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * 4, (void *) nullptr);
    glEnableVertexAttribArray(1); // texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * 4, (void *) (2 * 4));

    glGenTextures(1, &screen_framebuffer_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screen_framebuffer_texture);
    if (_openGlDebugAvailable) {
        glObjectLabel(GL_TEXTURE, screen_framebuffer_texture, -1, "melonDS DS Screen Texture");
    }
    GLint filter = config.ScreenFilter() == ScreenFilter::Linear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, NDS_SCREEN_WIDTH * 3 + 1, NDS_SCREEN_HEIGHT * 2, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, nullptr);

    _needsRefresh = true;
}

void MelonDsDs::OpenGLRenderState::Render(
    melonDS::NDS& nds,
    const InputState& input,
    const CoreConfig& config,
    const ScreenLayoutData& screenLayout
) noexcept {
    ZoneScopedN(TracyFunction);
    TracyGpuZone(TracyFunction);
    glsm_ctl(GLSM_CTL_STATE_BIND, nullptr);

    // Tell OpenGL that we want to draw to (and read from) the screen framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, glsm_get_current_framebuffer());

    if (_needsRefresh) {
        InitFrameState(nds, config, screenLayout);
        _needsRefresh = false;
    }

    if (!nds.IsLidClosed() && input.CursorVisible()) {
        float cursorSize = config.CursorSize();
        ivec2 touch = input.TouchPosition();
        GL_ShaderConfig.cursorPos[0] = ((float) touch.x - cursorSize) / NDS_SCREEN_WIDTH;
        GL_ShaderConfig.cursorPos[1] = (((float) touch.y - cursorSize) / (NDS_SCREEN_WIDTH * 1.5f)) + 0.5f;
        GL_ShaderConfig.cursorPos[2] = ((float) touch.x + cursorSize) / NDS_SCREEN_WIDTH;
        GL_ShaderConfig.cursorPos[3] = (((float) touch.y + cursorSize) / ((float) NDS_SCREEN_WIDTH * 1.5f)) + 0.5f;
        GL_ShaderConfig.cursorVisible = true;
    } else {
        GL_ShaderConfig.cursorVisible = false;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    void *unibuf = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    if (unibuf) memcpy(unibuf, &GL_ShaderConfig, sizeof(GL_ShaderConfig));
    glUnmapBuffer(GL_UNIFORM_BUFFER);

    melonDS::OpenGL::UseShaderProgram(shader.data());

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, screenLayout.BufferWidth(), screenLayout.BufferHeight());

    glActiveTexture(GL_TEXTURE0);

    if (nds.GPU.GetRenderer3D().Accelerated)
    {
        // hardware-accelerated render
        static_cast<melonDS::GLRenderer&>(nds.GPU.GetRenderer3D()).GetCompositor().BindOutputTexture(nds.GPU.FrontBuffer);
    }

    // Set the filtering mode for the active texture
    // For simplicity, we'll just use the same filter for both minification and magnification
    GLint filter = config.ScreenFilter() == ScreenFilter::Linear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);

    glFlush();

    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr);

    retro::video_refresh(
        RETRO_HW_FRAME_BUFFER_VALID,
        screenLayout.BufferWidth(),
        screenLayout.BufferHeight(),
        0
    );
    TracyGpuCollect;
}

void MelonDsDs::OpenGLRenderState::ContextDestroyed() {
    ZoneScopedN(TracyFunction);
//    TracyGpuZone(TracyFunction);
    retro::debug(TracyFunction);
    glsm_ctl(GLSM_CTL_STATE_CONTEXT_DESTROY, nullptr);
    _openGlDebugAvailable = false;
    _needsRefresh = false;
    _contextInitialized = false;
    shader = {};
    screen_framebuffer_texture = 0;
    screen_vertices = {};
    vertexCount = 0;
    vao = 0;
    vbo = 0;
    GL_ShaderConfig = {};
    ubo = 0;
    _lastResolutionScale = std::nullopt;
    _lastScreenFilter = std::nullopt;
    _lastBetterPolygonSplitting = std::nullopt;
}

void MelonDsDs::OpenGLRenderState::Apply(const CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    retro::debug(TracyFunction);
    if (_lastBetterPolygonSplitting != config.BetterPolygonSplitting()) {
        _lastBetterPolygonSplitting = config.BetterPolygonSplitting();
        _needsRefresh = true;
    }

    if (_lastScreenFilter != config.ScreenFilter()) {
        _lastScreenFilter = config.ScreenFilter();
        _needsRefresh = true;
    }

    if (_lastResolutionScale != config.ScaleFactor()) {
        _lastResolutionScale = config.ScaleFactor();
        _needsRefresh = true;
    }
}

void MelonDsDs::OpenGLRenderState::InitFrameState(melonDS::NDS& nds, const CoreConfig& config, const ScreenLayoutData& screenLayout) noexcept {
    ZoneScopedN(TracyFunction);
    TracyGpuZone(TracyFunction);
    retro_assert(nds.GPU.GetRenderer3D().Accelerated);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    melonDS::GLRenderer& renderer = static_cast<melonDS::GLRenderer&>(nds.GPU.GetRenderer3D());
    renderer.SetRenderSettings(config.BetterPolygonSplitting(), config.ScaleFactor());

    GL_ShaderConfig.uScreenSize = screenLayout.BufferSize();
    GL_ShaderConfig.u3DScale = screenLayout.Scale();
    GL_ShaderConfig.cursorPos = vec4(-1);

    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    void *unibuf = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    if (unibuf) memcpy(unibuf, &GL_ShaderConfig, sizeof(GL_ShaderConfig));
    glUnmapBuffer(GL_UNIFORM_BUFFER);

    InitVertices(screenLayout);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(screen_vertices), screen_vertices.data());
}

void MelonDsDs::OpenGLRenderState::InitVertices(const ScreenLayoutData& screenLayout) noexcept {
    ZoneScopedN(TracyFunction);
    ScreenLayout layout = screenLayout.Layout();
    HybridSideScreenDisplay hybridSideScreenDisplay = screenLayout.HybridSmallScreenLayout();
    vertexCount = GetVertexCount(layout, hybridSideScreenDisplay);

    const array<vec2, 12>& transformedPoints = screenLayout.TransformedScreenPoints();
    array<unsigned, 18> indexes = GetPositionIndexes(layout);

    // melonDS's OpenGL renderer draws both screens into a single texture,
    // the top being laid above the bottom without any gap.

    switch (layout) {
        case ScreenLayout::TurnRight:
        case ScreenLayout::TurnLeft:
        case ScreenLayout::UpsideDown:
        case ScreenLayout::TopBottom:
        case ScreenLayout::LeftRight:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                // Top screen
                screen_vertices[i] = {
                    .position = transformedPoints[indexes[i]],
                    .texcoord = TOP_SCREEN_TEXCOORDS[i],
                };

                // Touch screen
                screen_vertices[i + VERTEXES_PER_SCREEN] = {
                    .position = transformedPoints[indexes[i + VERTEXES_PER_SCREEN]],
                    .texcoord = BOTTOM_SCREEN_TEXCOORDS[i],
                };
            }
            break;
        case ScreenLayout::BottomTop:
        case ScreenLayout::RightLeft:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                // Top screen
                screen_vertices[i] = {
                    .position = transformedPoints[indexes[i]],
                    .texcoord = BOTTOM_SCREEN_TEXCOORDS[i],
                };

                // Touch screen
                screen_vertices[i + VERTEXES_PER_SCREEN] = {
                    .position = transformedPoints[indexes[i + VERTEXES_PER_SCREEN]],
                    .texcoord = TOP_SCREEN_TEXCOORDS[i],
                };
            }
            break;
        case ScreenLayout::TopOnly:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                screen_vertices[i] = {
                    .position = transformedPoints[indexes[i]],
                    .texcoord = TOP_SCREEN_TEXCOORDS[i],
                };
            }
            break;
        case ScreenLayout::BottomOnly:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                screen_vertices[i] = {
                    .position = transformedPoints[indexes[i]],
                    .texcoord = BOTTOM_SCREEN_TEXCOORDS[i],
                };
            }
            break;
        case ScreenLayout::HybridTop:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                // Hybrid screen
                screen_vertices[i] = {
                    .position = transformedPoints[indexes[i]],
                    .texcoord = TOP_SCREEN_TEXCOORDS[i],
                };

                // Bottom screen
                screen_vertices[i + VERTEXES_PER_SCREEN] = {
                    .position = transformedPoints[indexes[i + VERTEXES_PER_SCREEN]],
                    .texcoord = BOTTOM_SCREEN_TEXCOORDS[i],
                };

                // Top screen
                screen_vertices[i + 2*VERTEXES_PER_SCREEN] = {
                    .position = transformedPoints[indexes[i + 2*VERTEXES_PER_SCREEN]],
                    .texcoord = TOP_SCREEN_TEXCOORDS[i],
                };
                // (Won't be rendered if hybridSideScreenDisplay == HybridSideScreenDisplay::One)
            }
            break;
        case ScreenLayout::HybridBottom:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                // Hybrid screen
                screen_vertices[i] = {
                    .position = transformedPoints[indexes[i]],
                    .texcoord = BOTTOM_SCREEN_TEXCOORDS[i],
                };

                // Top screen
                screen_vertices[i + VERTEXES_PER_SCREEN] = {
                    .position = transformedPoints[indexes[i + VERTEXES_PER_SCREEN]],
                    .texcoord = TOP_SCREEN_TEXCOORDS[i],
                };

                // Bottom screen
                screen_vertices[i + 2*VERTEXES_PER_SCREEN] = {
                    .position = transformedPoints[indexes[i + 2*VERTEXES_PER_SCREEN]],
                    .texcoord = BOTTOM_SCREEN_TEXCOORDS[i],
                };
                // (Won't be rendered if hybridSideScreenDisplay == HybridSideScreenDisplay::One)
            }
            break;
    }
}
