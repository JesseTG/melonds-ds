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
  
#include <utility>

#include <NDS.h>
#include <GPU_Soft.h>
#include <retro_assert.h>

#include "config/config.hpp"
#include "exceptions.hpp"
#include "format.hpp"
#include "message/error.hpp"
#include "render/software.hpp"
#include "screenlayout.hpp"

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include <GPU_OpenGL.h>
#include "render/opengl.hpp"
#endif

void MelonDsDs::ApplyRendererSettings(melonDS::NDS& nds, const CoreConfig& config) noexcept {
    melonDS::RendererSettings settings {
        .ScaleFactor = static_cast<int>(config.ScaleFactor()),
        .Threaded = config.ThreadedSoftRenderer(),
        .HiresCoordinates = config.HiresCoordinates(), // Compute renderer only
        .BetterPolygons = config.BetterPolygonSplitting(), // Legacy OpenGL renderer only
    };

    nds.GetRenderer().SetRenderSettings(settings);
}


void MelonDsDs::RenderStateWrapper::Render(
    melonDS::NDS& nds,
    const InputState& input,
    const CoreConfig& config,
    const ScreenLayoutData& screenLayout
) noexcept {
    if (_renderState) {
        _renderState->Render(nds, input, config, screenLayout);
    }
}

/// Puts melonDS's software renderer back in place
/// so that its OpenGL renderer releases its objects while the context it made them in is still current.
///
/// Destroying an \c OpenGLRenderState tells the frontend that we're done with its context,
/// and the frontend is free to destroy and recreate it before the core runs again.
/// RetroArch does exactly that, from within the \c SET_SYSTEM_AV_INFO call
/// that reports the new render mode's geometry.
/// melonDS's renderer would then delete its OpenGL object names in a context
/// that has since given those same names to the frontend's own textures and shaders,
/// which stops the frontend from drawing anything at all (menus and overlays included).
static void ReleaseOpenGlRenderer(melonDS::NDS* nds) noexcept {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (!nds || !dynamic_cast<melonDS::GLRenderer*>(&nds->GetRenderer())) {
        // If there's no console yet, or if it isn't rendering with OpenGL...
        return;
    }

    retro::debug("Releasing melonDS's OpenGL renderer while its context is still current");
    nds->SetRenderer(std::make_unique<melonDS::SoftRenderer>(*nds));
#endif
}

void MelonDsDs::RenderStateWrapper::Render(
    const error::ErrorScreen& error,
    const CoreConfig& config,
    const ScreenLayoutData& screenLayout
) noexcept {
    SetRenderer(config, nullptr);
    static_cast<SoftwareRenderState*>(_renderState.get())->Render(error, config, screenLayout);
}

void MelonDsDs::RenderStateWrapper::RequestSoftwareFallback(std::string message) noexcept {
    _fallbackMessage = std::move(message);
    _softwareFallbackRequested = true;

#ifdef HAVE_COMPUTE_RENDERER
    if (auto* glState = dynamic_cast<OpenGLRenderState*>(_renderState.get());
        glState && glState->Mode() == RenderMode::Compute) {
        // Don't ask this frontend for an OpenGL 4.3 context again;
        // it's already shown that it can't run the compute renderer.
        _computeUnsupported = true;
    }
#endif
}

void MelonDsDs::RenderStateWrapper::Apply(const CoreConfig& config, melonDS::NDS* nds) noexcept {
    SetRenderer(config, nds);
}


void MelonDsDs::RenderStateWrapper::SetRenderer(const CoreConfig& config, melonDS::NDS* nds) {
    RenderMode wanted = config.ConfiguredRenderer();

#ifdef HAVE_COMPUTE_RENDERER
    if (wanted == RenderMode::Compute && _computeUnsupported) {
        // This frontend already failed to run the compute renderer once;
        // don't keep asking it for an OpenGL 4.3 context every time a setting changes.
        wanted = RenderMode::Software;
    }
#endif

    switch (wanted) {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#ifdef HAVE_COMPUTE_RENDERER
        case RenderMode::Compute:
#endif
        case RenderMode::OpenGl: {
            auto* glState = dynamic_cast<OpenGLRenderState*>(_renderState.get());
            if (glState && glState->Mode() == wanted) {
                // If we already have this OpenGL renderer configured...
                break;
            }

            if (glState && glState->CanHost(wanted)) {
                // The context we already have is new enough for the renderer we're switching to,
                // so keep it. Asking for a new one may not actually get us a new one:
                // RetroArch rebuilds its OpenGL context only when the core's maximum geometry changes,
                // which it doesn't when switching between two OpenGL renderers,
                // so the context reset we'd be waiting for would never come.
                retro::debug("Switching to the {} renderer on the context we already have", wanted);
                glState->SetMode(wanted);
                break;
            }

            // Each OpenGL render mode asks the frontend for a different context version,
            // so switching to one the current context can't drive means tearing down the old state
            // (which tells the frontend we're done with its context)
            // before requesting the new one.
            ReleaseOpenGlRenderer(nds);
            _renderState.reset();

            if (auto state = OpenGLRenderState::New(wanted)) {
                _renderState = std::move(state);
                retro::debug("Initialized {} render state", wanted);
                break;
            }

            retro::set_warn_message("Failed to initialize {} render state, falling back to software mode.", wanted);
            [[fallthrough]];
        }
#endif
        case RenderMode::Software:
        default: {
            // (Render modes this build doesn't offer end up here too)
            if (dynamic_cast<SoftwareRenderState*>(_renderState.get()) != nullptr) {
                // If we already have the software renderer configured...
                break;
            }

            ReleaseOpenGlRenderer(nds);
            _renderState = std::make_unique<SoftwareRenderState>(config);
            retro::debug("Initialized software render state");
            break;
        }
    }

    retro_assert(_renderState != nullptr);
}

void MelonDsDs::RenderStateWrapper::UpdateRenderer(const CoreConfig& config, melonDS::NDS& nds) noexcept {
    assert(_renderState != nullptr);

    if (dynamic_cast<SoftwareRenderState*>(_renderState.get())) {
        // If we're configured to use the software renderer...
        if (!dynamic_cast<melonDS::SoftRenderer*>(&nds.GetRenderer())) {
            // ...but we aren't using it yet...
            nds.SetRenderer(std::make_unique<melonDS::SoftRenderer>(nds));
        }

        ApplyRendererSettings(nds, config);
        return;
    }

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    auto* glRender = dynamic_cast<OpenGLRenderState*>(_renderState.get());
    bool rendererIsStale =
        // melonDS isn't using an OpenGL renderer at all (a new console starts with the software one)...
        !dynamic_cast<melonDS::GLRenderer*>(&nds.GetRenderer())
        // ...or it's using the OpenGL renderer we've since switched away from
        || (glRender && glRender->InstalledMode() != glRender->Mode());

    if (glRender && glRender->Ready() && rendererIsStale) {
        // If we're configured to use an OpenGL renderer and its context is up,
        // but melonDS isn't using that renderer yet...
        // (If the context isn't up yet, OpenGLRenderState::ContextReset will install the renderer once it is.)
        retro::debug("Initializing {} renderer", glRender->Mode());

        // Any renderer already in place is replaced here, while its context is still current
        nds.SetRenderer(std::make_unique<melonDS::GLRenderer>(nds, glRender->UsesComputeRenderer()));

        // melonDS installs its own software renderer if the one we gave it failed to start,
        // so that's how we find out whether this worked.
        if (dynamic_cast<melonDS::GLRenderer*>(&nds.GetRenderer())) {
            retro::debug("Initialized {} renderer.", glRender->Mode());
            glRender->MarkRendererInstalled();
            ApplyRendererSettings(nds, config);
            glRender->RequestRefresh();
        } else {
            retro::set_warn_message("Failed to initialize {} renderer, falling back to software mode.", glRender->Mode());
            _renderState = std::make_unique<SoftwareRenderState>(config);
            ApplyRendererSettings(nds, config);
        }
    }
#endif
}

void MelonDsDs::RenderStateWrapper::ContextReset(melonDS::NDS& nds, const CoreConfig& config) {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    auto* glRenderState = dynamic_cast<OpenGLRenderState*>(_renderState.get());
    if (!glRenderState) {
        return;
    }

    try {
        glRenderState->ContextReset(nds, config);
    }
    catch (const opengl_exception& e) {
        // We're inside the frontend's context_reset callback,
        // which is no place to tell it we've stopped using OpenGL;
        // remember to do that at the start of the next frame instead.
        retro::error("{}", e.what());
        _fallbackMessage = e.user_message();
        _softwareFallbackRequested = true;

#ifdef HAVE_COMPUTE_RENDERER
        if (glRenderState->Mode() == RenderMode::Compute) {
            _computeUnsupported = true;
        }
#endif
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

std::string MelonDsDs::RenderStateWrapper::FallBackToSoftware(const CoreConfig& config, melonDS::NDS* nds) noexcept {
    _softwareFallbackRequested = false;

    ReleaseOpenGlRenderer(nds);

    // Destroying an OpenGLRenderState tells the frontend we're done with its context
    _renderState.reset();
    _renderState = std::make_unique<SoftwareRenderState>(config);
    retro::debug("Fell back to the software render state");

    return std::exchange(_fallbackMessage, {});
}

std::optional<MelonDsDs::RenderMode> MelonDsDs::RenderStateWrapper::GetRenderMode() const noexcept {
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    if (dynamic_cast<SoftwareRenderState*>(_renderState.get()))
        return RenderMode::Software;

    if (auto* glState = dynamic_cast<OpenGLRenderState*>(_renderState.get()))
        return glState->Mode();

    return std::nullopt;
#else
    return _renderState ? std::make_optional(RenderMode::Software) : std::nullopt;
#endif
}
