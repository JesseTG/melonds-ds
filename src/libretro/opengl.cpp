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

#include <libretro.h>
#include <glsm/glsm.h>
#include <glsm/glsmsym.h>

#include <GPU.h>
#include <OpenGLSupport.h>

#include "screenlayout.hpp"
#include "input.hpp"
#include "environment.hpp"
#include "config.hpp"
#include "render.hpp"

namespace melonds::opengl {
    bool refresh_opengl = true;
    static bool context_initialized = false;
    static GLuint shader[3];
    static GLuint screen_framebuffer_texture;
    static float screen_vertices[72];
    static GLuint vao, vbo;
    struct shaders {
        // Declared within an anonymous struct so we can initialize them later in the file
        static const char *_vertex_shader;
        static const char *_fragment_shader;
    };

    static struct {
        GLfloat uScreenSize[2];
        u32 u3DScale;
        u32 uFilterMode;
        GLfloat cursorPos[4];

    } GL_ShaderConfig;
    static GLuint ubo;

    static void context_reset();

    static void context_destroy();

    static bool context_framebuffer_lock(void *data);

    static bool setup_opengl();

    static void setup_opengl_frame_state();
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

bool melonds::opengl::initialize() {
    retro::log(RETRO_LOG_DEBUG, "melonds::opengl::initialize()");
    glsm_ctx_params_t params = {nullptr};

    // melonds wants an opengl 3.1 context, so glcore is required for mesa compatibility
    params.context_type = RETRO_HW_CONTEXT_OPENGL_CORE;
    params.major = 3;
    params.minor = 1;
    params.context_reset = context_reset;
    params.context_destroy = context_destroy;
    params.environ_cb = retro::environment;
    params.stencil = false;
    params.framebuffer_lock = context_framebuffer_lock;

    return glsm_ctl(GLSM_CTL_STATE_CONTEXT_INIT, &params);
}

void melonds::opengl::render_frame(const InputState& state) {
    using melonds::screen_layout_data;
    glsm_ctl(GLSM_CTL_STATE_BIND, nullptr);

    int frontbuf = GPU::FrontBuffer;
    bool virtual_cursor = state.CursorEnabled();

    glBindFramebuffer(GL_FRAMEBUFFER, glsm_get_current_framebuffer());

    if (refresh_opengl) {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        setup_opengl_frame_state();
    }

    if (virtual_cursor) {
        float cursorSize = melonds::config::video::CursorSize();
        GL_ShaderConfig.cursorPos[0] = ((float) (state.TouchX()) - cursorSize) / (VIDEO_HEIGHT * 1.35f);
        GL_ShaderConfig.cursorPos[1] = (((float) (state.TouchY()) - cursorSize) / (VIDEO_WIDTH * 1.5f)) + 0.5f;
        GL_ShaderConfig.cursorPos[2] = ((float) (state.TouchX()) + cursorSize) / (VIDEO_HEIGHT * 1.35f);
        GL_ShaderConfig.cursorPos[3] = (((float) (state.TouchY()) + cursorSize) / ((float) VIDEO_WIDTH * 1.5f)) + 0.5f;

        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        void *unibuf = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
        if (unibuf) memcpy(unibuf, &GL_ShaderConfig, sizeof(GL_ShaderConfig));
        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

    OpenGL::UseShaderProgram(shader);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, screen_layout_data.BufferWidth(), screen_layout_data.BufferHeight());

    glActiveTexture(GL_TEXTURE0);

    GPU::CurGLCompositor->BindOutputTexture(frontbuf);

    GLint filter = config::video::ScreenFilter() == ScreenFilter::Linear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0,
                 screen_layout_data.HybridSmallScreenLayout() == SmallScreenLayout::SmallScreenDuplicate ? 18 : 12);

    glFlush();

    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr);

    retro::video_refresh(
        RETRO_HW_FRAME_BUFFER_VALID,
        screen_layout_data.BufferWidth(),
        screen_layout_data.BufferHeight(),
        0
    );

}


void melonds::opengl::deinitialize() {
    retro::log(RETRO_LOG_DEBUG, "melonds::opengl::deinitialize()");
    GPU::DeInitRenderer();
    GPU::InitRenderer(false);
}

static void melonds::opengl::context_reset() {
    retro::log(RETRO_LOG_DEBUG, "melonds::opengl::context_reset()");
    if (UsingOpenGl() && GPU3D::CurrentRenderer) { // If we're using OpenGL, but there's already a renderer in place...
        retro::log(RETRO_LOG_DEBUG, "GPU3D renderer is assigned; deinitializing it before resetting the context.");
        GPU::DeInitRenderer();
    }

    // These glsm_ctl calls always succeed
    glsm_ctl(GLSM_CTL_STATE_CONTEXT_RESET, nullptr);
    glsm_ctl(GLSM_CTL_STATE_SETUP, nullptr);
    glsm_ctl(GLSM_CTL_STATE_BIND, nullptr);

    // Renderer might be software, but we might also still be blitting with OpenGL
    GPU::InitRenderer(static_cast<int>(melonds::render::CurrentRenderer()));

    bool success = setup_opengl();

    glsm_ctl(GLSM_CTL_STATE_UNBIND, nullptr); // Always succeeds
    context_initialized = success;

    if (success) {
        retro::log(RETRO_LOG_DEBUG, "OpenGL context reset successfully.");
    } else {
        retro::log(RETRO_LOG_ERROR, "OpenGL context reset failed.");
    }

    // TODO: Signal that deferred initialization can now take place
    // (in case different frontends have different behavior)
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

static bool melonds::opengl::setup_opengl() {
    retro::log(RETRO_LOG_DEBUG, "melonds::opengl::setup_opengl()");

    if (!OpenGL::BuildShaderProgram(shaders::_vertex_shader, shaders::_fragment_shader, shader, "LibretroShader"))
        return false;

    glBindAttribLocation(shader[2], 0, "vPosition");
    glBindAttribLocation(shader[2], 1, "vTexcoord");
    glBindFragDataLocation(shader[2], 0, "oColor");

    if (!OpenGL::LinkShaderProgram(shader))
        return false;

    GLuint uni_id;

    uni_id = glGetUniformBlockIndex(shader[2], "uConfig");
    glUniformBlockBinding(shader[2], uni_id, 16);

    glUseProgram(shader[2]);
    uni_id = glGetUniformLocation(shader[2], "ScreenTex");
    glUniform1i(uni_id, 0);

    memset(&GL_ShaderConfig, 0, sizeof(GL_ShaderConfig));

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GL_ShaderConfig), &GL_ShaderConfig, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 16, ubo);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), nullptr, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * 4, (void *) nullptr);
    glEnableVertexAttribArray(1); // texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * 4, (void *) (2 * 4));

    glGenTextures(1, &screen_framebuffer_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screen_framebuffer_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, 256 * 3 + 1, 192 * 2, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, nullptr);

    refresh_opengl = true;

    return true;
}

void melonds::opengl::setup_opengl_frame_state(void) {

    refresh_opengl = false;
    GPU::RenderSettings render_settings = melonds::config::video::RenderSettings();
    GPU::SetRenderSettings(static_cast<int>(Renderer::OpenGl), render_settings);

    GL_ShaderConfig.uScreenSize[0] = (float) screen_layout_data.BufferWidth();
    GL_ShaderConfig.uScreenSize[1] = (float) screen_layout_data.BufferHeight();
    GL_ShaderConfig.u3DScale = config::video::ScaleFactor();
    GL_ShaderConfig.cursorPos[0] = -1.0f;
    GL_ShaderConfig.cursorPos[1] = -1.0f;
    GL_ShaderConfig.cursorPos[2] = -1.0f;
    GL_ShaderConfig.cursorPos[3] = -1.0f;

    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    void *unibuf = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    if (unibuf) memcpy(unibuf, &GL_ShaderConfig, sizeof(GL_ShaderConfig));
    glUnmapBuffer(GL_UNIFORM_BUFFER);

    float screen_width = (float) screen_layout_data.ScreenWidth();
    float screen_height = (float) screen_layout_data.ScreenHeight();
    float screen_gap = (float) screen_layout_data.ScaledScreenGap();

    float top_screen_x = 0.0f;
    float top_screen_y = 0.0f;
    float top_screen_scale = 1.0f;

    float bottom_screen_x = 0.0f;
    float bottom_screen_y = 0.0f;
    float bottom_screen_scale = 1.0f;

    float primary_x = 0.0f;
    float primary_y = 0.0f;
    float primary_tex_v0_x = 0.0f;
    float primary_tex_v0_y = 0.0f;
    float primary_tex_v1_x = 0.0f;
    float primary_tex_v1_y = 0.0f;
    float primary_tex_v2_x = 0.0f;
    float primary_tex_v2_y = 0.0f;
    float primary_tex_v3_x = 0.0f;
    float primary_tex_v3_y = 0.0f;
    float primary_tex_v4_x = 0.0f;
    float primary_tex_v4_y = 0.0f;
    float primary_tex_v5_x = 0.0f;
    float primary_tex_v5_y = 0.0f;

    const float pixel_pad = 1.0f / (192 * 2 + 2);

    // TODO: Implement rotated and upside-down layouts
    switch (screen_layout_data.EffectiveLayout()) {
        case ScreenLayout::TopBottom:
            bottom_screen_y = screen_height + screen_gap;
            break;
        case ScreenLayout::BottomTop:
            top_screen_y = screen_height + screen_gap;
            break;
        case ScreenLayout::LeftRight:
            bottom_screen_x = screen_width;
            break;
        case ScreenLayout::RightLeft:
            top_screen_x = screen_width;
            break;
        case ScreenLayout::TopOnly:
            bottom_screen_y = screen_height; // Meh, let's just hide it
            break;
        case ScreenLayout::BottomOnly:
            top_screen_y = screen_height; // ditto
            break;
        case ScreenLayout::HybridTop:
            primary_x = screen_width * screen_layout_data.HybridRatio();
            primary_y = screen_height * screen_layout_data.HybridRatio();

            primary_tex_v0_x = 0.0f;
            primary_tex_v0_y = 0.0f;
            primary_tex_v1_x = 0.0f;
            primary_tex_v1_y = 0.5f - pixel_pad;
            primary_tex_v2_x = 1.0f;
            primary_tex_v2_y = 0.5f - pixel_pad;
            primary_tex_v3_x = 0.0f;
            primary_tex_v3_y = 0.0f;
            primary_tex_v4_x = 1.0f;
            primary_tex_v4_y = 0.0f;
            primary_tex_v5_x = 1.0f;
            primary_tex_v5_y = 0.5f - pixel_pad;

            break;
        case ScreenLayout::HybridBottom:
            primary_x = screen_width * screen_layout_data.HybridRatio();
            primary_y = screen_height * screen_layout_data.HybridRatio();

            primary_tex_v0_x = 0.0f;
            primary_tex_v0_y = 0.5f + pixel_pad;
            primary_tex_v1_x = 0.0f;
            primary_tex_v1_y = 1.0f;
            primary_tex_v2_x = 1.0f;
            primary_tex_v2_y = 1.0f;
            primary_tex_v3_x = 0.0f;
            primary_tex_v3_y = 0.5f + pixel_pad;
            primary_tex_v4_x = 1.0f;
            primary_tex_v4_y = 0.5f + pixel_pad;
            primary_tex_v5_x = 1.0f;
            primary_tex_v5_y = 01.0;

            break;
    }

#define SETVERTEX(i, x, y, t_x, t_y) \
    do {                               \
        screen_vertices[(4 * i) + 0] = x; \
        screen_vertices[(4 * i) + 1] = y; \
        screen_vertices[(4 * i) + 2] = t_x; \
        screen_vertices[(4 * i) + 3] = t_y; \
    } while (false)

    ScreenLayout layout = screen_layout_data.EffectiveLayout();
    SmallScreenLayout smallScreenLayout = screen_layout_data.HybridSmallScreenLayout();
    if (screen_layout_data.IsHybridLayout()) {
        //Primary Screen
        SETVERTEX(0, 0.0f, 0.0f, primary_tex_v0_x, primary_tex_v0_y); // top left
        SETVERTEX(1, 0.0f, primary_y, primary_tex_v1_x, primary_tex_v1_y); // bottom left
        SETVERTEX(2, primary_x, primary_y, primary_tex_v2_x, primary_tex_v2_y); // bottom right
        SETVERTEX(3, 0.0f, 0.0f, primary_tex_v3_x, primary_tex_v3_y); // top left
        SETVERTEX(4, primary_x, 0.0f, primary_tex_v4_x, primary_tex_v4_y); // top right
        SETVERTEX(5, primary_x, primary_y, primary_tex_v5_x, primary_tex_v5_y); // bottom right

        //Top screen
        if (smallScreenLayout == SmallScreenLayout::SmallScreenTop && layout == ScreenLayout::HybridTop) {
            SETVERTEX(6, primary_x, 0.0f, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(7, primary_x, 0.0f + screen_height, 0.0f, 1.0f); // bottom left
            SETVERTEX(8, primary_x + screen_width, 0.0f + screen_height, 1.0f, 1.0f); // bottom right
            SETVERTEX(9, primary_x, 0.0f, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(10, primary_x + screen_width, 0.0f, 1.0f, 0.5f + pixel_pad); // top right
            SETVERTEX(11, primary_x + screen_width, 0.0f + screen_height, 1.0f, 1.0f); // bottom right
        } else if (smallScreenLayout == SmallScreenLayout::SmallScreenDuplicate
                   || (layout == ScreenLayout::HybridBottom && smallScreenLayout == SmallScreenLayout::SmallScreenTop)) {
            SETVERTEX(6, primary_x, 0.0f, 0.0f, 0.0f); // top left
            SETVERTEX(7, primary_x, 0.0f + screen_height, 0.0f, 0.5f - pixel_pad); // bottom left
            SETVERTEX(8, primary_x + screen_width, 0.0f + screen_height, 1.0f, 0.5f - pixel_pad); // bottom right
            SETVERTEX(9, primary_x, 0.0f, 0.0f, 0.0f); // top left
            SETVERTEX(10, primary_x + screen_width, 0.0f, 1.0f, 0.0f); // top right
            SETVERTEX(11, primary_x + screen_width, 0.0f + screen_height, 1.0f, 0.5f - pixel_pad); // bottom right
        }


        //Bottom Screen
        if (smallScreenLayout == SmallScreenLayout::SmallScreenBottom &&
            layout == ScreenLayout::HybridTop) {
            SETVERTEX(6, primary_x, primary_y - screen_height, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(7, primary_x, primary_y, 0.0f, 1.0f); // bottom left
            SETVERTEX(8, primary_x + screen_width, primary_y, 1.0f, 1.0f); // bottom right
            SETVERTEX(9, primary_x, primary_y - screen_height, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(10, primary_x + screen_width, primary_y - screen_height, 1.0f, 0.5f + pixel_pad); // top right
            SETVERTEX(11, primary_x + screen_width, primary_y, 1.0f, 1.0f); // bottom right
        } else if (smallScreenLayout == SmallScreenLayout::SmallScreenBottom && layout == ScreenLayout::HybridBottom) {
            SETVERTEX(6, primary_x, primary_y - screen_height, 0.0f, 0.0f); // top left
            SETVERTEX(7, primary_x, primary_y, 0.0f, 0.5f - pixel_pad); // bottom left
            SETVERTEX(8, primary_x + screen_width, primary_y, 1.0f, 0.5f - pixel_pad); // bottom right
            SETVERTEX(9, primary_x, primary_y - screen_height, 0.0f, 0.0f); // top left
            SETVERTEX(10, primary_x + screen_width, primary_y - screen_height, 1.0f, 0.0f); // top right
            SETVERTEX(11, primary_x + screen_width, primary_y, 1.0f, 0.5f - pixel_pad); // bottom right
        } else if (smallScreenLayout == SmallScreenLayout::SmallScreenDuplicate) {
            SETVERTEX(12, primary_x, primary_y - screen_height, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(13, primary_x, primary_y, 0.0f, 1.0f); // bottom left
            SETVERTEX(14, primary_x + screen_width, primary_y, 1.0f, 1.0f); // bottom right
            SETVERTEX(15, primary_x, primary_y - screen_height, 0.0f, 0.5f + pixel_pad); // top left
            SETVERTEX(16, primary_x + screen_width, primary_y - screen_height, 1.0f, 0.5f + pixel_pad); // top right
            SETVERTEX(17, primary_x + screen_width, primary_y, 1.0f, 1.0f); // bottom right
        }
    } else {
        // top screen
        SETVERTEX(0, top_screen_x, top_screen_y, 0.0f, 0.0f); // top left
        SETVERTEX(1, top_screen_x, top_screen_y + screen_height * top_screen_scale, 0.0f,
                  0.5f - pixel_pad); // bottom left
        SETVERTEX(2, top_screen_x + screen_width * top_screen_scale, top_screen_y + screen_height * top_screen_scale,
                  1.0f, 0.5f - pixel_pad); // bottom right
        SETVERTEX(3, top_screen_x, top_screen_y, 0.0f, 0.0f); // top left
        SETVERTEX(4, top_screen_x + screen_width * top_screen_scale, top_screen_y, 1.0f, 0.0f); // top right
        SETVERTEX(5, top_screen_x + screen_width * top_screen_scale, top_screen_y + screen_height * top_screen_scale,
                  1.0f, 0.5f - pixel_pad); // bottom right

        // bottom screen
        SETVERTEX(6, bottom_screen_x, bottom_screen_y, 0.0f, 0.5f + pixel_pad); // top left
        SETVERTEX(7, bottom_screen_x, bottom_screen_y + screen_height * bottom_screen_scale, 0.0f, 1.0f); // bottom left
        SETVERTEX(8, bottom_screen_x + screen_width * bottom_screen_scale,
                  bottom_screen_y + screen_height * bottom_screen_scale, 1.0f, 1.0f); // bottom right
        SETVERTEX(9, bottom_screen_x, bottom_screen_y, 0.0f, 0.5f + pixel_pad); // top left
        SETVERTEX(10, bottom_screen_x + screen_width * bottom_screen_scale, bottom_screen_y, 1.0f,
                  0.5f + pixel_pad); // top right
        SETVERTEX(11, bottom_screen_x + screen_width * bottom_screen_scale,
                  bottom_screen_y + screen_height * bottom_screen_scale, 1.0f, 1.0f); // bottom right
    }

    // top screen


    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(screen_vertices), screen_vertices);
}

static bool melonds::opengl::context_framebuffer_lock(void *data) {
    return false;
}

// TODO: Store in a .glsl file, but use CMake to embed it
const char *melonds::opengl::shaders::_vertex_shader = R"(#version 140
layout(std140) uniform uConfig
{
    vec2 uScreenSize;
    uint u3DScale;
    uint uFilterMode;
    vec4 cursorPos;
};
in vec2 pos;
in vec2 texcoord;
smooth out vec2 fTexcoord;
void main()
{
    vec4 fpos;
    fpos.xy = ((pos * 2.0) / uScreenSize) - 1.0;
    fpos.y *= -1;
    fpos.z = 0.0;
    fpos.w = 1.0;
    gl_Position = fpos;
    fTexcoord = texcoord;
}
)";

// TODO: Store in a .glsl file, but use CMake to embed it
const char *melonds::opengl::shaders::_fragment_shader = R"(#version 140
layout(std140) uniform uConfig
{
    vec2 uScreenSize;
    uint u3DScale;
    uint uFilterMode;
    vec4 cursorPos;
};
uniform sampler2D ScreenTex;
smooth in vec2 fTexcoord;
out vec4 oColor;
void main()
{
    vec4 pixel = texture(ScreenTex, fTexcoord);
    // virtual cursor so you can see where you touch
    if(fTexcoord.y >= 0.5 && fTexcoord.y <= 1.0) {
        if(cursorPos.x <= fTexcoord.x && cursorPos.y <= fTexcoord.y && cursorPos.z >= fTexcoord.x && cursorPos.w >= fTexcoord.y) {
            pixel = vec4(1.0 - pixel.r, 1.0 - pixel.g, 1.0 - pixel.b, pixel.a);
        }
    }
    oColor = vec4(pixel.bgr, 1.0);
}
)";