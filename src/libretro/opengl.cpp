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

using glm::vec2;
using glm::vec4;

// HACK: Defined in glsm.c, but we need to peek into it occasionally
extern struct retro_hw_render_callback hw_render;

static const char* const SHADER_PROGRAM_NAME = "melonDS DS Shader Program";

namespace melonds::opengl {
    // TODO: Introduce a OpenGlState struct to hold all of these variables
    static bool openGlDebugAvailable = false;
    bool refresh_opengl = true;
    static bool context_initialized = false;
    static GLuint shader[3];
    static GLuint screen_framebuffer_texture;
    static float screen_vertices[72];
    static GLuint vao, vbo;

    static struct {
        vec2 uScreenSize;
        u32 u3DScale;
        u32 uFilterMode;
        vec4 cursorPos;
    } GL_ShaderConfig;
    static GLuint ubo;

    static void ContextReset() noexcept;

    static void context_destroy();

    static void SetupOpenGl();

    static void InitializeFrameState(const ScreenLayoutData& screenLayout) noexcept;
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
    retro::log(RETRO_LOG_DEBUG, "melonds::opengl::Initialize()");
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
    retro_assert(melonds::render::CurrentRenderer() == melonds::Renderer::OpenGl);
    glsm_ctl(GLSM_CTL_STATE_BIND, nullptr);

    int frontbuf = GPU::FrontBuffer;
    bool virtual_cursor = state.CursorEnabled();

    // Tell OpenGL that we want to draw to (and read from) the screen framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, glsm_get_current_framebuffer());

    if (refresh_opengl) {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        InitializeFrameState(screenLayout);
    }

    if (virtual_cursor) {
        float cursorSize = melonds::config::video::CursorSize();
        GL_ShaderConfig.cursorPos[0] = ((float) (state.TouchX()) - cursorSize) / (NDS_SCREEN_HEIGHT * 1.35f);
        GL_ShaderConfig.cursorPos[1] = (((float) (state.TouchY()) - cursorSize) / (NDS_SCREEN_WIDTH * 1.5f)) + 0.5f;
        GL_ShaderConfig.cursorPos[2] = ((float) (state.TouchX()) + cursorSize) / (NDS_SCREEN_HEIGHT * 1.35f);
        GL_ShaderConfig.cursorPos[3] = (((float) (state.TouchY()) + cursorSize) / ((float) NDS_SCREEN_WIDTH * 1.5f)) + 0.5f;

        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        void *unibuf = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
        if (unibuf) memcpy(unibuf, &GL_ShaderConfig, sizeof(GL_ShaderConfig));
        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

    OpenGL::UseShaderProgram(shader);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, screenLayout.BufferWidth(), screenLayout.BufferHeight());

    glActiveTexture(GL_TEXTURE0);

    GPU::CurGLCompositor->BindOutputTexture(frontbuf);

    // Set the filtering mode for the active texture
    // For simplicity, we'll just use the same filter for both minification and magnification
    GLint filter = config::video::ScreenFilter() == ScreenFilter::Linear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0,
                 screenLayout.HybridSmallScreenLayout() == SmallScreenLayout::SmallScreenDuplicate ? 18 : 12);

    glFlush();

    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr);

    retro::video_refresh(
        RETRO_HW_FRAME_BUFFER_VALID,
        screenLayout.BufferWidth(),
        screenLayout.BufferHeight(),
        0
    );

}


void melonds::opengl::deinitialize() {
    retro::log(RETRO_LOG_DEBUG, "melonds::opengl::deinitialize()");
    GPU::DeInitRenderer();
    GPU::InitRenderer(false);
}

static void melonds::opengl::ContextReset() noexcept try {
    retro::debug("melonds::opengl::ContextReset()");
    if (UsingOpenGl() && GPU3D::CurrentRenderer) { // If we're using OpenGL, but there's already a renderer in place...
        retro::debug("GPU3D renderer is assigned; deinitializing it before resetting the context.");
        GPU::DeInitRenderer();
    }

    // Initialize all OpenGL function pointers
    glsm_ctl(GLSM_CTL_STATE_CONTEXT_RESET, nullptr);

    // Initialize global OpenGL resources (e.g. VAOs) and get config info (e.g. limits)
    glsm_ctl(GLSM_CTL_STATE_SETUP, nullptr);

    // Start using global OpenGL structures
    glsm_ctl(GLSM_CTL_STATE_BIND, nullptr);

    GPU::InitRenderer(static_cast<int>(melonds::render::CurrentRenderer()));

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
    retro::shutdown();
}
catch (...) {
    context_initialized = false;
    retro::set_error_message("OpenGL context initialization failed with an unknown error. Please report this issue.");
    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr);
    retro::shutdown();
}

static void melonds::opengl::context_destroy() {
    retro::log(RETRO_LOG_DEBUG, "melonds::opengl::context_destroy()");
    glsm_ctl(GLSM_CTL_STATE_BIND, nullptr);
    glDeleteTextures(1, &screen_framebuffer_texture);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    OpenGL::DeleteShaderProgram(shader);
    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr);
}

// Sets up OpenGL resources specific to melonDS
static void melonds::opengl::SetupOpenGl() {
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
        glObjectLabel(GL_BUFFER, vbo, -1, "melonDS DS Screen VBO");
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

void melonds::opengl::InitializeFrameState(const ScreenLayoutData& screenLayout) noexcept {

    refresh_opengl = false;
    GPU::RenderSettings render_settings = melonds::config::video::RenderSettings();
    GPU::SetRenderSettings(static_cast<int>(Renderer::OpenGl), render_settings);

    GL_ShaderConfig.uScreenSize = screenLayout.BufferSize();
    GL_ShaderConfig.u3DScale = config::video::ScaleFactor();
    GL_ShaderConfig.cursorPos = vec4(-1);

    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    void *unibuf = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    if (unibuf) memcpy(unibuf, &GL_ShaderConfig, sizeof(GL_ShaderConfig));
    glUnmapBuffer(GL_UNIFORM_BUFFER);

    vec2 screenSize = screenLayout.ScreenSize();
    float screen_gap = (float) screenLayout.ScaledScreenGap();

    // TODO: Replace these two variables with a transformation matrix
    vec2 topScreen = vec2(0, 0);
    float top_screen_scale = 1.0f;

    // TODO: Replace these variables with a transformation matrix
    vec2 bottomScreen = vec2(0, 0);
    float bottom_screen_scale = 1.0f;

    vec2 primary = vec2(0, 0);
    vec2 primaryTexV0 = vec2(0, 0);
    vec2 primaryTexV1 = vec2(0, 0);
    vec2 primaryTexV2 = vec2(0, 0);
    vec2 primaryTexV3 = vec2(0, 0);
    vec2 primaryTexV4 = vec2(0, 0);
    vec2 primaryTexV5 = vec2(0, 0);

    const float pixel_pad = 1.0f / (192 * 2 + 2);

    switch (screenLayout.Layout()) {
        case ScreenLayout::TurnRight:
        case ScreenLayout::TurnLeft:
        case ScreenLayout::UpsideDown:
        case ScreenLayout::TopBottom:
            bottomScreen.y = screenSize.y + screen_gap;
            break;
        case ScreenLayout::BottomTop:
            topScreen.y = screenSize.y + screen_gap;
            break;
        case ScreenLayout::LeftRight:
            bottomScreen.x = screenSize.x;
            break;
        case ScreenLayout::RightLeft:
            topScreen.x = screenSize.x;
            break;
        case ScreenLayout::TopOnly:
            bottomScreen.y = screenSize.y; // Meh, let's just hide it
            break;
        case ScreenLayout::BottomOnly:
            topScreen.y = screenSize.y; // ditto
            break;
        case ScreenLayout::HybridTop:
            primary.x = screenSize.x * screenLayout.HybridRatio();
            primary.y = screenSize.y * screenLayout.HybridRatio();

            primaryTexV0.x = 0.0f;
            primaryTexV0.y = 0.0f;
            primaryTexV1.x = 0.0f;
            primaryTexV1.y = 0.5f - pixel_pad;
            primaryTexV2.x = 1.0f;
            primaryTexV2.y = 0.5f - pixel_pad;
            primaryTexV3.x = 0.0f;
            primaryTexV3.y = 0.0f;
            primaryTexV4.x = 1.0f;
            primaryTexV4.y = 0.0f;
            primaryTexV5.x = 1.0f;
            primaryTexV5.y = 0.5f - pixel_pad;

            break;
        case ScreenLayout::HybridBottom:
            primary.x = screenSize.x * screenLayout.HybridRatio();
            primary.y = screenSize.y * screenLayout.HybridRatio();

            primaryTexV0.x = 0.0f;
            primaryTexV0.y = 0.5f + pixel_pad;
            primaryTexV1.x = 0.0f;
            primaryTexV1.y = 1.0f;
            primaryTexV2.x = 1.0f;
            primaryTexV2.y = 1.0f;
            primaryTexV3.x = 0.0f;
            primaryTexV3.y = 0.5f + pixel_pad;
            primaryTexV4.x = 1.0f;
            primaryTexV4.y = 0.5f + pixel_pad;
            primaryTexV5.x = 1.0f;
            primaryTexV5.y = 01.0;

            break;
    }

#define SETVERTEX(i, x, y, t_x, t_y) \
    do {                               \
        screen_vertices[(4 * i) + 0] = x; \
        screen_vertices[(4 * i) + 1] = y; \
        screen_vertices[(4 * i) + 2] = t_x; \
        screen_vertices[(4 * i) + 3] = t_y; \
    } while (false)

    ScreenLayout layout = screenLayout.Layout();
    SmallScreenLayout smallScreenLayout = screenLayout.HybridSmallScreenLayout();
    if (screenLayout.IsHybridLayout()) {
        //Primary Screen
        SETVERTEX(0, 0.0f, 0.0f, primaryTexV0.x, primaryTexV0.y); // top left
        SETVERTEX(1, 0.0f, primary.y, primaryTexV1.x, primaryTexV1.y); // bottom left
        SETVERTEX(2, primary.x, primary.y, primaryTexV2.x, primaryTexV2.y); // bottom right
        SETVERTEX(3, 0.0f, 0.0f, primaryTexV3.x, primaryTexV3.y); // top left
        SETVERTEX(4, primary.x, 0.0f, primaryTexV4.x, primaryTexV4.y); // top right
        SETVERTEX(5, primary.x, primary.y, primaryTexV5.x, primaryTexV5.y); // bottom right

        //Top screen
        if (smallScreenLayout == SmallScreenLayout::SmallScreenTop && layout == ScreenLayout::HybridTop) {
            SETVERTEX(6, primary.x, 0.0f, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(7, primary.x, 0.0f + screenSize.y, 0.0f, 1.0f); // bottom left
            SETVERTEX(8, primary.x + screenSize.x, 0.0f + screenSize.y, 1.0f, 1.0f); // bottom right
            SETVERTEX(9, primary.x, 0.0f, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(10, primary.x + screenSize.x, 0.0f, 1.0f, 0.5f + pixel_pad); // top right
            SETVERTEX(11, primary.x + screenSize.x, 0.0f + screenSize.y, 1.0f, 1.0f); // bottom right
        } else if (smallScreenLayout == SmallScreenLayout::SmallScreenDuplicate
                   || (layout == ScreenLayout::HybridBottom && smallScreenLayout == SmallScreenLayout::SmallScreenTop)) {
            SETVERTEX(6, primary.x, 0.0f, 0.0f, 0.0f); // top left
            SETVERTEX(7, primary.x, 0.0f + screenSize.y, 0.0f, 0.5f - pixel_pad); // bottom left
            SETVERTEX(8, primary.x + screenSize.x, 0.0f + screenSize.y, 1.0f, 0.5f - pixel_pad); // bottom right
            SETVERTEX(9, primary.x, 0.0f, 0.0f, 0.0f); // top left
            SETVERTEX(10, primary.x + screenSize.x, 0.0f, 1.0f, 0.0f); // top right
            SETVERTEX(11, primary.x + screenSize.x, 0.0f + screenSize.y, 1.0f, 0.5f - pixel_pad); // bottom right
        }


        //Bottom Screen
        if (smallScreenLayout == SmallScreenLayout::SmallScreenBottom &&
            layout == ScreenLayout::HybridTop) {
            SETVERTEX(6, primary.x, primary.y - screenSize.y, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(7, primary.x, primary.y, 0.0f, 1.0f); // bottom left
            SETVERTEX(8, primary.x + screenSize.x, primary.y, 1.0f, 1.0f); // bottom right
            SETVERTEX(9, primary.x, primary.y - screenSize.y, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(10, primary.x + screenSize.x, primary.y - screenSize.y, 1.0f, 0.5f + pixel_pad); // top right
            SETVERTEX(11, primary.x + screenSize.x, primary.y, 1.0f, 1.0f); // bottom right
        } else if (smallScreenLayout == SmallScreenLayout::SmallScreenBottom && layout == ScreenLayout::HybridBottom) {
            SETVERTEX(6, primary.x, primary.y - screenSize.y, 0.0f, 0.0f); // top left
            SETVERTEX(7, primary.x, primary.y, 0.0f, 0.5f - pixel_pad); // bottom left
            SETVERTEX(8, primary.x + screenSize.x, primary.y, 1.0f, 0.5f - pixel_pad); // bottom right
            SETVERTEX(9, primary.x, primary.y - screenSize.y, 0.0f, 0.0f); // top left
            SETVERTEX(10, primary.x + screenSize.x, primary.y - screenSize.y, 1.0f, 0.0f); // top right
            SETVERTEX(11, primary.x + screenSize.x, primary.y, 1.0f, 0.5f - pixel_pad); // bottom right
        } else if (smallScreenLayout == SmallScreenLayout::SmallScreenDuplicate) {
            SETVERTEX(12, primary.x, primary.y - screenSize.y, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(13, primary.x, primary.y, 0.0f, 1.0f); // bottom left
            SETVERTEX(14, primary.x + screenSize.x, primary.y, 1.0f, 1.0f); // bottom right
            SETVERTEX(15, primary.x, primary.y - screenSize.y, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(16, primary.x + screenSize.x, primary.y - screenSize.y, 1.0f, 0.5f + pixel_pad); // top right
            SETVERTEX(17, primary.x + screenSize.x, primary.y, 1.0f, 1.0f); // bottom right
        }
    } else {
        // top screen
        SETVERTEX(0, topScreen.x, topScreen.y, 0.0f, 0.0f); // top left
        SETVERTEX(1, topScreen.x, topScreen.y + screenSize.y * top_screen_scale, 0.0f,
                  0.5f - pixel_pad); // bottom left
        SETVERTEX(2, topScreen.x + screenSize.x * top_screen_scale, topScreen.y + screenSize.y * top_screen_scale,
                  1.0f, 0.5f - pixel_pad); // bottom right
        SETVERTEX(3, topScreen.x, topScreen.y, 0.0f, 0.0f); // top left
        SETVERTEX(4, topScreen.x + screenSize.x * top_screen_scale, topScreen.y, 1.0f, 0.0f); // top right
        SETVERTEX(5, topScreen.x + screenSize.x * top_screen_scale, topScreen.y + screenSize.y * top_screen_scale,
                  1.0f, 0.5f - pixel_pad); // bottom right

        // bottom screen
        SETVERTEX(6, bottomScreen.x, bottomScreen.y, 0.0f, 0.5f + pixel_pad); // top left
        SETVERTEX(7, bottomScreen.x, bottomScreen.y + screenSize.y * bottom_screen_scale, 0.0f, 1.0f); // bottom left
        SETVERTEX(8, bottomScreen.x + screenSize.x * bottom_screen_scale,
                  bottomScreen.y + screenSize.y * bottom_screen_scale, 1.0f, 1.0f); // bottom right
        SETVERTEX(9, bottomScreen.x, bottomScreen.y, 0.0f, 0.5f + pixel_pad); // top left
        SETVERTEX(10, bottomScreen.x + screenSize.x * bottom_screen_scale, bottomScreen.y, 1.0f,
                  0.5f + pixel_pad); // top right
        SETVERTEX(11, bottomScreen.x + screenSize.x * bottom_screen_scale,
                  bottomScreen.y + screenSize.y * bottom_screen_scale, 1.0f, 1.0f); // bottom right
    }

    // top screen


    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(screen_vertices), screen_vertices);
}
