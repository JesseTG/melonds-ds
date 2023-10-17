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
#include <gfx/gl_capabilities.h>
#include <libretro.h>
#include <glsm/glsm.h>
#include <retro_assert.h>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <GPU.h>
#include <OpenGLSupport.h>

#include "embedded/melondsds_fragment_shader.h"
#include "embedded/melondsds_vertex_shader.h"
#include "PlatformOGLPrivate.h"
#include "exceptions.hpp"
#include "screenlayout.hpp"
#include "input.hpp"
#include "environment.hpp"
#include "config.hpp"
#include "render.hpp"
#include "tracy.hpp"

#ifdef TRACY_ENABLE
#include <tracy/TracyOpenGL.hpp>
#endif

using std::array;
using glm::ivec2;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using melonds::ScreenLayout;

// HACK: Defined in glsm.c, but we need to peek into it occasionally
extern struct retro_hw_render_callback hw_render;

static const char* const SHADER_PROGRAM_NAME = "melonDS DS Shader Program";

namespace melonds::opengl {
    constexpr float PIXEL_PAD = 1.0f / (NDS_SCREEN_HEIGHT * 2 + 2);
    constexpr unsigned VERTEXES_PER_SCREEN = 6;
    constexpr array<vec2, VERTEXES_PER_SCREEN> TOP_SCREEN_TEXCOORDS {
        vec2(0), // northwest
        vec2(0, 0.5f - PIXEL_PAD), // southwest
        vec2(1, 0.5f - PIXEL_PAD), // southeast
        vec2(0), //northwest
        vec2(1, 0), // northeast
        vec2(1, 0.5f - PIXEL_PAD), // southeast
    };
    constexpr array<vec2, VERTEXES_PER_SCREEN> BOTTOM_SCREEN_TEXCOORDS {
        vec2(0, 0.5f + PIXEL_PAD), // northwest
        vec2(0, 1), // southwest
        vec2(1), // southeast
        vec2(0, 0.5f + PIXEL_PAD), // northwest
        vec2(1, 0.5f + PIXEL_PAD), // northeast
        vec2(1), // southeast
    };
    struct Vertex {
        vec2 position;
        vec2 texcoord;
    };

    static_assert(sizeof(Vertex) == sizeof(vec2::value_type) * 4);
    // TODO: Introduce a OpenGlState struct to hold all of these variables
    static bool openGlDebugAvailable = false;
    bool refresh_opengl = true;
    static bool context_initialized = false;
    static GLuint shader[3];
    static GLuint screen_framebuffer_texture;
    static Vertex screen_vertices[18];
    static unsigned vertexCount = 0;
    static GLuint vao, vbo;

    static struct {
        vec2 uScreenSize;
        u32 u3DScale;
        u32 uFilterMode;
        vec4 cursorPos;
        bool cursorVisible;
    } GL_ShaderConfig;
    static GLuint ubo;

    static void ContextReset() noexcept;

    static void context_destroy();

    static void SetupOpenGl();

    static void InitializeFrameState(const ScreenLayoutData& screenLayout) noexcept;
    static void InitializeVertices(const ScreenLayoutData& screenLayout) noexcept;
}

constexpr unsigned GetVertexCount(ScreenLayout layout, melonds::HybridSideScreenDisplay hybridScreen) noexcept {
    switch (layout) {
        case ScreenLayout::TopOnly:
        case ScreenLayout::BottomOnly:
            return 6; // 1 screen, 2 triangles
        case ScreenLayout::HybridTop:
        case ScreenLayout::HybridBottom:
            if (hybridScreen == melonds::HybridSideScreenDisplay::Both)
                return 18; // 3 screens, 6 triangles
            [[fallthrough]];
        default:
            return 12; // 2 screens, 4 triangles
    }
}

bool melonds::opengl::ContextInitialized() {
    return context_initialized;
}

bool melonds::opengl::UsingOpenGl() {
    if (melonds::render::CurrentRenderer() == melonds::Renderer::OpenGl) {
        return true;
    }

    return false;
}

void melonds::opengl::RequestOpenGlRefresh() {
    refresh_opengl = true;
}

bool melonds::opengl::Initialize() noexcept {
    ZoneScopedN("melonds::opengl::Initialize");
    retro::debug("melonds::opengl::Initialize()");
    glsm_ctx_params_t params = {};

    // melonds wants an opengl 3.1 context, so glcore is required for mesa compatibility
    params.context_type = RETRO_HW_CONTEXT_OPENGL_CORE;
    params.major = 3;
    params.minor = 1;
    params.context_reset = ContextReset;
    params.context_destroy = context_destroy;
    params.environ_cb = retro::environment;
    params.stencil = false;
    params.framebuffer_lock = nullptr;

#ifndef NDEBUG
    hw_render.debug_context = true;
#endif

    bool ok = glsm_ctl(GLSM_CTL_STATE_CONTEXT_INIT, &params);

#ifndef NDEBUG
    retro_assert(hw_render.debug_context);
#endif

    gl_query_core_context_set(hw_render.context_type == RETRO_HW_CONTEXT_OPENGL_CORE || hw_render.context_type == RETRO_HW_CONTEXT_OPENGL);

    return ok;
}

void melonds::opengl::Render(const InputState& state, const ScreenLayoutData& screenLayout) noexcept {
    ZoneScopedN("melonds::opengl::Render");
    TracyGpuZone("melonds::opengl::Render");
    retro_assert(melonds::render::CurrentRenderer() == melonds::Renderer::OpenGl);
    glsm_ctl(GLSM_CTL_STATE_BIND, nullptr);

    // Tell OpenGL that we want to draw to (and read from) the screen framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, glsm_get_current_framebuffer());

    if (refresh_opengl) {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        InitializeFrameState(screenLayout);
    }

    if (state.CursorVisible()) {
        float cursorSize = melonds::config::screen::CursorSize();
        ivec2 touch = state.TouchPosition();
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

    OpenGL::UseShaderProgram(shader);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, screenLayout.BufferWidth(), screenLayout.BufferHeight());

    glActiveTexture(GL_TEXTURE0);

    GPU::CurGLCompositor->BindOutputTexture(GPU::FrontBuffer);

    // Set the filtering mode for the active texture
    // For simplicity, we'll just use the same filter for both minification and magnification
    GLint filter = config::video::ScreenFilter() == ScreenFilter::Linear ? GL_LINEAR : GL_NEAREST;
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

void melonds::opengl::deinitialize() {
    retro::debug("melonds::opengl::deinitialize()");
    GPU::DeInitRenderer();
    GPU::InitRenderer(false);
}

static void melonds::opengl::ContextReset() noexcept try {
    ZoneScopedN("melonds::opengl::ContextReset");
    retro::debug("melonds::opengl::ContextReset()");
    if (UsingOpenGl() && GPU3D::CurrentRenderer) { // If we're using OpenGL, but there's already a renderer in place...
        retro::debug("GPU3D renderer is assigned; deinitializing it before resetting the context.");
        GPU::DeInitRenderer();
    }

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

    {
        ZoneScopedN("GPU::InitRenderer");
        TracyGpuZone("GPU::InitRenderer");
        GPU::InitRenderer(static_cast<int>(melonds::render::CurrentRenderer()));
    }

    SetupOpenGl();
    context_initialized = true;

    // Stop using OpenGL structures
    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr); // Always succeeds

    retro::debug("OpenGL context reset successfully.");
}
catch (const melonds::emulator_exception& e) {
    context_initialized = false;
    retro::error(e.what());
    retro::set_error_message(e.user_message());
    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr);
    retro::shutdown();
}
catch (const std::exception& e) {
    context_initialized = false;
    retro::set_error_message(e.what());
    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr);
    // TODO: Instead of shutting down, fall back to the software renderer
    retro::shutdown();
}
catch (...) {
    context_initialized = false;
    retro::set_error_message("OpenGL context initialization failed with an unknown error. Please report this issue.");
    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr);
    retro::shutdown();
}

static void melonds::opengl::context_destroy() {
    ZoneScopedN("melonds::opengl::context_destroy");
    retro::debug("melonds::opengl::context_destroy()");
    glsm_ctl(GLSM_CTL_STATE_BIND, nullptr);
    glDeleteTextures(1, &screen_framebuffer_texture);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    OpenGL::DeleteShaderProgram(shader);
    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr);
}

// Sets up OpenGL resources specific to melonDS
static void melonds::opengl::SetupOpenGl() {
    ZoneScopedN("melonds::opengl::SetupOpenGl");
    TracyGpuZone("melonds::opengl::SetupOpenGl");
    retro::debug("melonds::opengl::SetupOpenGl()");

    openGlDebugAvailable = gl_check_capability(GL_CAPS_DEBUG);
    if (openGlDebugAvailable) {
        retro::debug("OpenGL debugging extensions are available");
    }

    if (!OpenGL::BuildShaderProgram(embedded_melondsds_vertex_shader, embedded_melondsds_fragment_shader, shader, SHADER_PROGRAM_NAME))
        throw melonds::shader_compilation_failed_exception("Failed to compile melonDS DS shaders.");

    if (openGlDebugAvailable) {
        glObjectLabel(GL_SHADER, shader[0], -1, "melonDS DS Vertex Shader");
        glObjectLabel(GL_SHADER, shader[1], -1, "melonDS DS Fragment Shader");
        glObjectLabel(GL_PROGRAM, shader[2], -1, SHADER_PROGRAM_NAME);
    }

    glBindAttribLocation(shader[2], 0, "vPosition");
    glBindAttribLocation(shader[2], 1, "vTexcoord");
    glBindFragDataLocation(shader[2], 0, "oColor");

    if (!OpenGL::LinkShaderProgram(shader))
        throw melonds::shader_compilation_failed_exception("Failed to link compiled shaders.");

    GLuint uConfigBlockIndex = glGetUniformBlockIndex(shader[2], "uConfig");
    glUniformBlockBinding(shader[2], uConfigBlockIndex, 16); // TODO: Where does 16 come from? It's not a size.

    glUseProgram(shader[2]);
    GLuint uni_id = glGetUniformLocation(shader[2], "ScreenTex");
    glUniform1i(uni_id, 0);

    memset(&GL_ShaderConfig, 0, sizeof(GL_ShaderConfig));

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    if (openGlDebugAvailable) {
        glObjectLabel(GL_BUFFER, ubo, -1, "melonDS DS Shader Config UBO");
    }
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GL_ShaderConfig), &GL_ShaderConfig, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 16, ubo);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (openGlDebugAvailable) {
        glObjectLabel(GL_BUFFER, vbo, -1, "melonDS DS Screen Vertex Buffer");
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), nullptr, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    if (openGlDebugAvailable) {
        glObjectLabel(GL_VERTEX_ARRAY, vao, -1, "melonDS DS Screen VAO");
    }
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * 4, (void *) nullptr);
    glEnableVertexAttribArray(1); // texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * 4, (void *) (2 * 4));

    glGenTextures(1, &screen_framebuffer_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screen_framebuffer_texture);
    if (openGlDebugAvailable) {
        glObjectLabel(GL_TEXTURE, screen_framebuffer_texture, -1, "melonDS DS Screen Texture");
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, NDS_SCREEN_WIDTH * 3 + 1, NDS_SCREEN_HEIGHT * 2, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, nullptr);

    refresh_opengl = true;
}

constexpr array<unsigned, 18> GetPositionIndexes(melonds::ScreenLayout layout) noexcept {
    using melonds::opengl::VERTEXES_PER_SCREEN;

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

static void melonds::opengl::InitializeVertices(const ScreenLayoutData& screenLayout) noexcept {
    ZoneScopedN("melonds::opengl::InitializeVertices");
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

void melonds::opengl::InitializeFrameState(const ScreenLayoutData& screenLayout) noexcept {
    ZoneScopedN("melonds::opengl::InitializeFrameState");
    TracyGpuZone("melonds::opengl::InitializeFrameState");
    refresh_opengl = false;
    GPU::RenderSettings render_settings = melonds::config::video::RenderSettings();
    GPU::SetRenderSettings(static_cast<int>(Renderer::OpenGl), render_settings);

    GL_ShaderConfig.uScreenSize = screenLayout.BufferSize();
    GL_ShaderConfig.u3DScale = screenLayout.Scale();
    GL_ShaderConfig.cursorPos = vec4(-1);

    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    void *unibuf = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    if (unibuf) memcpy(unibuf, &GL_ShaderConfig, sizeof(GL_ShaderConfig));
    glUnmapBuffer(GL_UNIFORM_BUFFER);

    InitializeVertices(screenLayout);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(screen_vertices), screen_vertices);
}
