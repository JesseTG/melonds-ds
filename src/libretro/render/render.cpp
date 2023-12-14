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

#include "render.hpp"

#include "PlatformOGLPrivate.h"

#include <NDS.h>
#include <GPU3D_Soft.h>
#include <retro_assert.h>

#include "config/config.hpp"
#include "message/error.hpp"
#include "render/software.hpp"
#include "screenlayout.hpp"

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include <GPU3D_OpenGL.h>
#include "render/opengl.hpp"
#endif


void MelonDsDs::RenderStateWrapper::Render(melonDS::NDS& nds, const InputState& input, const CoreConfig& config, const ScreenLayoutData& screenLayout) noexcept {
    if (_renderState) {
        _renderState->Render(nds, input, config, screenLayout);
    }
}

void MelonDsDs::RenderStateWrapper::Render(const error::ErrorScreen& error, const ScreenLayoutData& screenLayout) noexcept {
    SetRenderer(Renderer::Software);
    static_cast<SoftwareRenderState*>(_renderState.get())->Render(error, screenLayout);
}

void MelonDsDs::RenderStateWrapper::Apply(const CoreConfig& config) noexcept {
    SetRenderer(config.ConfiguredRenderer());

    _renderState->Apply(config);
}


void MelonDsDs::RenderStateWrapper::SetRenderer(Renderer renderer) {
    switch (renderer) {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
        case Renderer::OpenGl: {
            if (dynamic_cast<OpenGLRenderState*>(_renderState.get()) != nullptr) {
                // If we already have the OpenGL renderer configured...
                break;
            }

            if (auto state = OpenGLRenderState::New()) {
                _renderState = std::move(state);
                retro::debug("Initialized OpenGL render state");
                break;
            }

            retro::set_warn_message("Failed to initialize OpenGL render state, falling back to software mode.");
            [[fallthrough]];
        }
#endif
        case Renderer::Software: {
            if (dynamic_cast<SoftwareRenderState*>(_renderState.get()) != nullptr) {
                // If we already have the software renderer configured...
                break;
            }

            _renderState = std::make_unique<SoftwareRenderState>();
            retro::debug("Initialized software render state");
            break;
        }
    }

    retro_assert(_renderState != nullptr);
}

void MelonDsDs::RenderStateWrapper::UpdateRenderer(const CoreConfig& config, melonDS::NDS& nds) noexcept {
    assert(_renderState != nullptr);

    if (dynamic_cast<SoftwareRenderState*>(_renderState.get()) && nds.GPU.GetRenderer3D().Accelerated) {
        // If we're configured to use the software renderer, and we aren't already...
        nds.GPU.SetRenderer3D(std::make_unique<melonDS::SoftRenderer>(nds.GPU, config.ThreadedSoftRenderer()));
        return;
    }

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (dynamic_cast<OpenGLRenderState*>(_renderState.get()) && !nds.GPU.GetRenderer3D().Accelerated) {
        // If we're configured to use the OpenGL renderer, and we aren't already...

        if (auto renderer = melonDS::GLRenderer::New(nds.GPU)) {
            nds.GPU.SetRenderer3D(std::move(renderer));
        }
        else {
            retro::set_warn_message("Failed to initialize OpenGL renderer, falling back to software mode.");
            _renderState = std::make_unique<SoftwareRenderState>();
            nds.GPU.SetRenderer3D(std::make_unique<melonDS::SoftRenderer>(nds.GPU, config.ThreadedSoftRenderer()));
        }
    }
#endif
}

void MelonDsDs::RenderStateWrapper::ContextReset(melonDS::NDS& nds, const CoreConfig& config) {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (auto glRenderState = dynamic_cast<OpenGLRenderState*>(_renderState.get())) {
        glRenderState->ContextReset(nds, config);
    }
#endif
}

void MelonDsDs::RenderStateWrapper::ContextDestroyed() {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (auto glRenderState = dynamic_cast<OpenGLRenderState*>(_renderState.get())) {
        glRenderState->ContextDestroyed();
    }
#endif
}

std::optional<MelonDsDs::Renderer> MelonDsDs::RenderStateWrapper::GetRenderer() const noexcept {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (dynamic_cast<SoftwareRenderState*>(_renderState.get()))
        return Renderer::Software;

    if (dynamic_cast<OpenGLRenderState*>(_renderState.get()))
        return Renderer::OpenGl;

    return std::nullopt;
#else
    return _renderState ? std::make_optional(Renderer::Software) : std::nullopt;
#endif
}