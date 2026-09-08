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

#ifndef MELONDS_DS_RENDER_HPP
#define MELONDS_DS_RENDER_HPP

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include "config/types.hpp"

namespace melonDS {
    class NDS;
}

namespace MelonDsDs {
    class InputState;
    class ScreenLayoutData;
    class CoreConfig;

    namespace error {
        class ErrorScreen;
    }

    /// Applies \c config's renderer settings to whichever renderer \c nds is currently using.
    /// melonDS accepts all renderers' settings through one struct,
    /// then forwards each field to whichever renderer cares about it.
    void ApplyRendererSettings(melonDS::NDS& nds, const CoreConfig& config) noexcept;

    /// How far along a renderer is in compiling the shaders it has queued up.
    struct ShaderCompileProgress {
        /// How many shader programs are ready.
        int Compiled = 0;
        /// How many there are in total, or 0 if there was nothing to compile.
        int Total = 0;
        /// True if no shaders are left to compile.
        bool Done = true;
        /// True if a shader failed to compile, in which case this renderer is unusable.
        bool Failed = false;
    };

    class RenderState {
    public:
        virtual ~RenderState() noexcept = default;

        /// Returns true if all state necessary for rendering is ready.
        /// This includes the OpenGL context (if applicable) and the emulator's renderer.
        virtual bool Ready() const noexcept = 0;
        virtual void Render(melonDS::NDS& nds, const InputState& input, const CoreConfig& config, const ScreenLayoutData& screenLayout) noexcept = 0;
        virtual void RequestRefresh() noexcept {}

        /// Compiles shaders that melonDS's renderer has queued up,
        /// spending no more than \c budget on them.
        /// Only call while the frontend's OpenGL context is current, if there is one.
        virtual ShaderCompileProgress CompileShaders(melonDS::NDS& nds, std::chrono::microseconds budget) noexcept {
            return {};
        }
    };

    class RenderStateWrapper {
    public:
        bool Ready() const noexcept { return _renderState && _renderState->Ready(); }
        void Render(melonDS::NDS& nds, const InputState& input, const CoreConfig& config, const ScreenLayoutData& screenLayout) noexcept;
        void Render(const error::ErrorScreen& error, const CoreConfig& config, const ScreenLayoutData& screenLayout) noexcept;
        void RequestRefresh() noexcept {
            if (_renderState) {
                _renderState->RequestRefresh();
            }
        }

        /// \see RenderState::CompileShaders
        ShaderCompileProgress CompileShaders(melonDS::NDS& nds, std::chrono::microseconds budget) noexcept {
            return _renderState ? _renderState->CompileShaders(nds, budget) : ShaderCompileProgress {};
        }

        /// Installs the render state that \c config asks for.
        /// \param nds The console whose renderer should give up the frontend's OpenGL context
        /// if this switches away from OpenGL, or \c nullptr if there isn't one yet.
        void Apply(const CoreConfig& config, melonDS::NDS* nds) noexcept;
        [[gnu::cold]] void UpdateRenderer(const CoreConfig& config, melonDS::NDS& nds) noexcept;
        void ContextReset(melonDS::NDS& nds, const CoreConfig& config);
        void ContextDestroyed();
        std::optional<RenderMode> GetRenderMode() const noexcept;

        /// True if the OpenGL render state failed in a way that's only recoverable
        /// by switching to software rendering, and the switch hasn't happened yet.
        /// Check at the start of each frame;
        /// the failure is usually detected inside a frontend callback,
        /// where the switch itself isn't safe to perform.
        [[nodiscard]] bool SoftwareFallbackRequested() const noexcept { return _softwareFallbackRequested; }

        /// Asks for the switch to software rendering that \c SoftwareFallbackRequested reports,
        /// showing \c message to the player once it happens.
        [[gnu::cold]] void RequestSoftwareFallback(std::string message) noexcept;

        /// Replaces the current render state with the software one
        /// and returns the message to show the player explaining why.
        /// Only call from \c retro_run;
        /// this tells the frontend we've stopped rendering with OpenGL.
        /// \see Apply for what \c nds is for.
        [[gnu::cold]] std::string FallBackToSoftware(const CoreConfig& config, melonDS::NDS* nds) noexcept;
    private:
        void SetRenderer(const CoreConfig& config, melonDS::NDS* nds);
        std::unique_ptr<RenderState> _renderState;
        bool _softwareFallbackRequested = false;
        /// Set once the frontend has shown it can't run the compute renderer,
        /// so later settings changes don't keep asking it for an OpenGL 4.3 context.
        bool _computeUnsupported = false;
        std::string _fallbackMessage;
    };
}

#endif //MELONDS_DS_RENDER_HPP
