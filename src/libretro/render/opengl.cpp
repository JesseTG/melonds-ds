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
#include <cstring>

#include <GPU3D_OpenGL.h>
#include <NDS.h>

#include <gfx/gl_capabilities.h>
#include <glsym/rglgen.h>
#include <retro_assert.h>
#include <embedded/melondsds_fragment_shader.h>
#include <embedded/melondsds_vertex_shader.h>

#include "../core/core.hpp"
#include "exceptions.hpp"
#include "format.hpp"
#include "screenlayout.hpp"
#include "tracy.hpp"

using glm::ivec2;
using glm::mat3;
using glm::vec2;
using glm::vec3;
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
        case ScreenLayout::LargescreenTop:
        case ScreenLayout::FlippedLargescreenBottom:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                indexes[i] = topPositionIndexes[i];
                indexes[i + VERTEXES_PER_SCREEN] = bottomPositionIndexes[i];
            }
            break;
        case ScreenLayout::RightLeft:
        case ScreenLayout::BottomTop:
        case ScreenLayout::LargescreenBottom:
        case ScreenLayout::FlippedLargescreenTop:
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
        case ScreenLayout::FlippedHybridTop:
            for (unsigned i = 0; i < VERTEXES_PER_SCREEN; ++i) {
                indexes[i] = hybridPositionIndexes[i];
                indexes[i + VERTEXES_PER_SCREEN] = bottomPositionIndexes[i];
                indexes[i + VERTEXES_PER_SCREEN*2] = topPositionIndexes[i];
            }
            break;
        case ScreenLayout::HybridBottom:
        case ScreenLayout::FlippedHybridBottom:
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
        case ScreenLayout::FlippedHybridTop:
        case ScreenLayout::FlippedHybridBottom:
            if (hybridScreen == MelonDsDs::HybridSideScreenDisplay::Both)
                return 18; // 3 screens, 6 triangles
        [[fallthrough]];
        default:
            return 12; // 2 screens, 4 triangles
    }
}

static const char* const SHADER_PROGRAM_NAME = "melonDS DS Shader Program";

// Index of the uniform buffer binding point used by the screen shader's uConfig block
constexpr GLuint SHADER_CONFIG_UBO_BINDING = 16; // TODO: Where does 16 come from? It's not a size.


std::unique_ptr<MelonDsDs::OpenGLRenderState> MelonDsDs::OpenGLRenderState::New() noexcept {
    ZoneScopedN(TracyFunction);
    try {
        return std::make_unique<OpenGLRenderState>();
    } catch (const opengl_not_initialized_exception& e) {
        retro::debug("OpenGL context could not be initialized: {}", e.what());
        return nullptr;
    }
}

MelonDsDs::OpenGLRenderState::OpenGLRenderState() {
    ZoneScopedN(TracyFunction);
    retro::debug(TracyFunction);

    // MelonDS needs at least OpenGL 3.2 for OpenGL renderer
    // (it doesn't use the legacy fixed-function pipeline)
    _hw_render.context_type = RETRO_HW_CONTEXT_OPENGL_CORE;
    _hw_render.version_major = 3;
    _hw_render.version_minor = 2;
    _hw_render.context_reset = HardwareContextReset;
    _hw_render.context_destroy = HardwareContextDestroyed;
    _hw_render.depth = true;
    _hw_render.stencil = false;
    _hw_render.bottom_left_origin = true;
    _hw_render.cache_context = false;

#ifndef NDEBUG
    _hw_render.debug_context = true;
#endif

    // If this succeeds, the frontend fills in _hw_render's function pointers
    // (but it won't call them until the context is reset)
    if (!retro::set_hw_render(_hw_render)) {
        throw opengl_not_initialized_exception();
    }

#ifndef NDEBUG
    retro_assert(_hw_render.debug_context);
#endif

    gl_query_core_context_set(_hw_render.context_type == RETRO_HW_CONTEXT_OPENGL_CORE);
}

MelonDsDs::OpenGLRenderState::~OpenGLRenderState() noexcept {
    retro::debug(TracyFunction);
    if (_contextInitialized) {
        TracyGpuZone(TracyFunction);
        BindState();
        glDeleteTextures(1, &screen_framebuffer_texture);

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ubo);
        glDeleteProgram(_screenProgram);
        UnbindState();

#if defined(HAVE_TRACY) && !defined(__APPLE__)
        _tracyCapture = std::nullopt;
#endif
    }
    gl_query_core_context_unset();

    // Disable OpenGL hardware rendering;
    // this may not actually tear down the OpenGL context
    // (i.e. the frame may still be presented with OpenGL),
    // but it does signal to the frontend that we're back to software rendering.
    retro_hw_render_callback none {};
    none.context_type = RETRO_HW_CONTEXT_NONE;
    retro::set_hw_render(none);
}

void MelonDsDs::OpenGLRenderState::ContextReset(melonDS::NDS& nds, const CoreConfig& config) {
    ZoneScopedN(TracyFunction);
    retro::debug(TracyFunction);

    // Initialize all OpenGL function pointers
    retro::debug("Initializing OpenGL function pointers");
    if (!_hw_render.get_proc_address || !_hw_render.get_current_framebuffer) {
        retro::error("Frontend didn't provide the get_proc_address and get_current_framebuffer callbacks");
        throw opengl_not_initialized_exception();
    }

    // The core doesn't link against OpenGL,
    // so the OpenGL 1.0/1.1 functions that glsym doesn't cover
    // must be resolved before we make our first OpenGL call
    // (see PlatformOGLPrivate.h)
    rglgen_resolve_symbols_custom(
        reinterpret_cast<rglgen_proc_address_t>(_hw_render.get_proc_address),
        melondsds_base_gl_symbol_map
    );

    unsigned missingSymbols = 0;
    for (const rglgen_sym_map* entry = melondsds_base_gl_symbol_map; entry->sym; ++entry) {
        void* address = nullptr;
        memcpy(&address, entry->ptr, sizeof(address));
        if (!address) {
            retro::error("Frontend couldn't resolve {}", entry->sym);
            ++missingSymbols;
        }
    }

    if (missingSymbols) {
        retro::error("Frontend couldn't resolve {} OpenGL functions, can't use the OpenGL renderer", missingSymbols);
        throw opengl_not_initialized_exception();
    }

    // Now resolve everything else that glsym knows about
    rglgen_resolve_symbols(_hw_render.get_proc_address);
    TracyGpuContext; // Must be called AFTER the function pointers are bound!

    const char *vendor   = (const char*)glGetString(GL_VENDOR);
    const char *rendererName = (const char*)glGetString(GL_RENDERER);
    const char *version  = (const char*)glGetString(GL_VERSION);

    retro::info("OpenGL version: {}", version ? version : "<null>");
    retro::info("OpenGL vendor: {}", vendor ? vendor : "<null>");
    retro::info("OpenGL renderer: {}", rendererName ? rendererName : "<null>");

    // Start using OpenGL on the frontend's framebuffer
    retro::debug("Binding GL state");
    BindState();
    retro::debug("Bound GL state");

    GLuint fbo = CurrentFramebuffer();
    retro_assert(glIsFramebuffer(fbo) == GL_TRUE);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    retro::debug("Current OpenGL framebuffer: id={}, status={}", fbo, static_cast<FormattedGLEnum>(status));

    // HACK: Makes the core resilient to context loss by cleaning up the stale OpenGL renderer
    // (The "correct" way to do this would be to add a Reinitialize() method to GLRenderer
    // that recreates all resources)
    nds.GPU.GPU3D.SetCurrentRenderer(std::make_unique<melonDS::SoftRenderer>());
    auto renderer = melonDS::GLRenderer::New();
    if (!renderer) {
        retro::error("Failed to initialize OpenGL renderer!");
        throw opengl_not_initialized_exception();
    }
    retro::debug("Constructed OpenGL renderer");
    renderer->SetRenderSettings(config.BetterPolygonSplitting(), config.ScaleFactor());
    retro::debug("Applied OpenGL renderer settings");
    nds.GPU.SetRenderer3D(std::move(renderer));
    retro::debug("Installed OpenGL renderer");

    SetUpCoreOpenGlState(config);
    retro::debug("Initialized core OpenGL state");
    _contextInitialized = true;

    // Stop using OpenGL structures
    UnbindState();
    retro::debug("Unbound GL state");

#if defined(HAVE_TRACY) && !defined(__APPLE__)
    if (tracy::ProfilerAvailable()) {
        // If we're using Tracy...
        retro::debug("Using Tracy, will capture OpenGL calls");
        _tracyCapture.emplace(_openGlDebugAvailable); // ...then get ready to capture OpenGL calls
    }
#endif

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

    // TODO: Check gl_check_capability for GL_CAPS_VAO and GL_CAPS_FBO

    bool shaderCompiled = melonDS::OpenGL::CompileVertexFragmentProgram(
        _screenProgram,
        embedded_melondsds_vertex_shader,
        embedded_melondsds_fragment_shader,
        SHADER_PROGRAM_NAME,
        {
            {"vPosition", 0},
            {"vTexcoord", 1},
        },
        {
            {"oColor", 0},
        }
    );

    if (!shaderCompiled)
        throw shader_compilation_failed_exception("Failed to compile and link melonDS DS screen shader program.");

    if (_openGlDebugAvailable) {
        // TODO: Fall back to glLabelObjectEXT if glObjectLabel isn't available
        glObjectLabel(GL_PROGRAM, _screenProgram, -1, SHADER_PROGRAM_NAME);
    }

    GLuint uConfigBlockIndex = glGetUniformBlockIndex(_screenProgram, "uConfig");
    glUniformBlockBinding(_screenProgram, uConfigBlockIndex, SHADER_CONFIG_UBO_BINDING);

    glUseProgram(_screenProgram);
    GLuint uni_id = glGetUniformLocation(_screenProgram, "ScreenTex");
    glUniform1i(uni_id, 0);

    memset(&GL_ShaderConfig, 0, sizeof(GL_ShaderConfig));

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    if (_openGlDebugAvailable) {
        glObjectLabel(GL_BUFFER, ubo, -1, "melonDS DS Shader Config UBO");
    }
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GL_ShaderConfig), &GL_ShaderConfig, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, SHADER_CONFIG_UBO_BINDING, ubo);

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
    retro_assert(nds.GetRenderer3D().Accelerated);

    // Binds the frontend's framebuffer, the screen shader, the VAO/VBO/UBO,
    // and resets whatever fixed-function state the frontend or melonDS may have changed
    BindState();

    melonDS::GLRenderer& renderer = static_cast<melonDS::GLRenderer&>(nds.GetRenderer3D());

    if (renderer.GetBetterPolygons() != config.BetterPolygonSplitting() || renderer.GetScaleFactor() != config.ScaleFactor())
        // If any of the OpenGL renderer's settings have changed...
        _needsRefresh = true;

    if (_needsRefresh) {
        InitFrameState(nds, config, screenLayout);
        _needsRefresh = false;
    }

    if (!nds.IsLidClosed() && input.CursorVisible()) {
        float cursorSize = config.CursorSize();
        ScreenLayout layout = screenLayout.Layout();
        ivec2 touch = clamp(input.ConsoleTouchPosition(), ivec2(0), ivec2(NDS_SCREEN_WIDTH - 1, NDS_SCREEN_HEIGHT - 1));

        bool secondaryTouchInBounds =
            (input.TouchPosition().x >= 0 && input.TouchPosition().x < NDS_SCREEN_WIDTH) &&
            (input.TouchPosition().y >= 0 && input.TouchPosition().y < NDS_SCREEN_HEIGHT);
        bool touchUsesHybrid =
            (layout == ScreenLayout::HybridBottom || layout == ScreenLayout::FlippedHybridBottom) &&
            (screenLayout.HybridSmallScreenLayout() == HybridSideScreenDisplay::One || !secondaryTouchInBounds);

        const mat3& touchScreenMatrix = touchUsesHybrid ? screenLayout.GetHybridScreenMatrix() : screenLayout.GetBottomScreenMatrix();

        vec3 p0 = touchScreenMatrix * vec3(vec2(touch) - vec2(cursorSize), 1.0f);
        vec3 p1 = touchScreenMatrix * vec3(vec2(touch) + vec2(cursorSize), 1.0f);
        float x0 = std::min(p0.x, p1.x);
        float y0 = std::min(p0.y, p1.y);
        float x1 = std::max(p0.x, p1.x);
        float y1 = std::max(p0.y, p1.y);
        GL_ShaderConfig.cursorPos = vec4(x0, y0, x1, y1);
        GL_ShaderConfig.cursorVisible = true;
    } else {
        GL_ShaderConfig.cursorVisible = false;
    }

    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GL_ShaderConfig), &GL_ShaderConfig);

    glViewport(0, 0, screenLayout.BufferWidth(), screenLayout.BufferHeight());

    // Bind melonDS's output texture to texture unit 0 (the active unit after BindState)
    renderer.BindOutputTexture(nds.GPU.FrontBuffer);

    // Set the filtering mode for the active texture
    // For simplicity, we'll just use the same filter for both minification and magnification
    GLint filter = config.ScreenFilter() == ScreenFilter::Linear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    if (nds.IsLidClosed()) [[unlikely]] {
        // If the emulated lid is closed, just draw a blank
        // so that there's no annoying flickering with some games
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    else {
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    glFlush();

    UnbindState();

#if defined(HAVE_TRACY) && !defined(__APPLE__)
    if (_tracyCapture) {
        // TODO: Expose the FBO that the emulator's GLRenderer uses for rendering, then pass it here
        _tracyCapture->CaptureFrame(CurrentFramebuffer(), config.ScaleFactor());
    }
#endif

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
    _openGlDebugAvailable = false;
    _needsRefresh = false;
    _contextInitialized = false;
    _screenProgram = 0;
    screen_framebuffer_texture = 0;
    screen_vertices = {};
    vertexCount = 0;
    vao = 0;
    vbo = 0;
    GL_ShaderConfig = {};
    ubo = 0;
    // TODO: Delete these objects, since the context hasn't been destroyed yet
    // (just in case it's not really destroyed afterwards)

#if defined(HAVE_TRACY) && !defined(__APPLE__)
    _tracyCapture = std::nullopt;
#endif
}

GLuint MelonDsDs::OpenGLRenderState::CurrentFramebuffer() const noexcept {
    retro_assert(_hw_render.get_current_framebuffer != nullptr);
    return static_cast<GLuint>(_hw_render.get_current_framebuffer());
}

// This and UnbindState replace the parts of libretro-common's glsm
// that melonDS DS actually used, before glsm was removed from libretro-common.
// Unlike glsm, nothing here is tracked through wrappers;
// melonDS DS and melonDS both call OpenGL directly,
// so we just apply the state we know the core needs.
void MelonDsDs::OpenGLRenderState::BindState() noexcept {
    ZoneScopedN(TracyFunction);
    TracyGpuZone(TracyFunction);

    // Draw to (and read from) the frontend's framebuffer.
    // Don't cache it; some frontends hand out a different one each frame.
    glBindFramebuffer(GL_FRAMEBUFFER, CurrentFramebuffer());

    // Our own resources (all 0 until SetUpCoreOpenGlState runs, which is fine)
    glUseProgram(_screenProgram);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBufferBase(GL_UNIFORM_BUFFER, SHADER_CONFIG_UBO_BINDING, ubo); // Also binds GL_UNIFORM_BUFFER itself

    // Fixed-function state that the screen blit depends on;
    // the frontend or melonDS's renderer may have left any of it in another state
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE); // For the depth clears in InitFrameState and the closed-lid path
    glClearColor(0, 0, 0, 0);

    // melonDS's output texture gets bound to unit 0 in Render
    glActiveTexture(GL_TEXTURE0);
}

void MelonDsDs::OpenGLRenderState::UnbindState() noexcept {
    ZoneScopedN(TracyFunction);
    TracyGpuZone(TracyFunction);

    // Everything below is state that melonDS DS or melonDS's OpenGL renderer sets,
    // reset to OpenGL's defaults.
    // melonDS's renderer sets all of it again every frame,
    // except the uniform buffer it binds to index 0 once at startup;
    // leave that index alone or its shaders will stop working.

    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, SHADER_CONFIG_UBO_BINDING, 0); // Also unbinds GL_UNIFORM_BUFFER itself

    // melonDS's renderer leaves its readback PBO bound,
    // which makes the frontend's own glReadPixels (e.g. for screenshots) fail
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);

    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendColor(0, 0, 0, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Covers the per-buffer masks melonDS sets with glColorMaski
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDepthRange(0, 1);
    glClearDepth(1);
    glClearColor(0, 0, 0, 0);
    glStencilFunc(GL_ALWAYS, 0, ~0u);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(~0u);
    glLineWidth(1);

    // melonDS's renderer and compositor only use texture units 0 and 1
    // (the texture cache binds 2D array textures to whichever unit is active)
    for (GLenum unit : {GL_TEXTURE1, GL_TEXTURE0}) {
        glActiveTexture(unit);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }
    // (Loop order leaves GL_TEXTURE0 active, which is the default)

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GL_ShaderConfig), &GL_ShaderConfig);

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
        case ScreenLayout::LargescreenTop:
        case ScreenLayout::FlippedLargescreenBottom:
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
        case ScreenLayout::LargescreenBottom:
        case ScreenLayout::FlippedLargescreenTop:
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
        case ScreenLayout::FlippedHybridTop:
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
        case ScreenLayout::FlippedHybridBottom:
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
