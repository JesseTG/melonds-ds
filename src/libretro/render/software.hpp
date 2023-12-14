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

#ifndef MELONDSDS_RENDER_SOFTWARE_HPP
#define MELONDSDS_RENDER_SOFTWARE_HPP

#include <optional>
#include <span>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

#include "buffer.hpp"
#include "render.hpp"
#include "screenlayout.hpp"
#include "retro/scaler.hpp"

namespace MelonDsDs {
    namespace error {
        class ErrorScreen;
    }

    class CoreConfig;

    class SoftwareRenderState final : public RenderState {
    public:
        SoftwareRenderState(const CoreConfig& config) noexcept;
        bool Ready() const noexcept override { return true; }
        void Render(
            melonDS::NDS& nds,
            const InputState& input,
            const CoreConfig& config,
            const ScreenLayoutData& screenLayout
        ) noexcept override;

        void Render(
            const error::ErrorScreen& error,
            const ScreenLayoutData& screenLayout
        ) noexcept;

        unsigned BufferWidth() const noexcept { return buffer.Width(); }
        unsigned BufferHeight() const noexcept { return buffer.Height(); }
        glm::uvec2 BufferSize() const noexcept { return buffer.Size(); }

    private:
        void CopyScreen(const uint32_t* src, glm::uvec2 destTranslation, ScreenLayout layout) noexcept;
        void DrawCursor(const InputState& input, const CoreConfig& config, const ScreenLayoutData& screenLayout) noexcept;
        void CombineScreens(
            std::span<const uint32_t, NDS_SCREEN_AREA<size_t>> topBuffer,
            std::span<const uint32_t, NDS_SCREEN_AREA<size_t>> bottomBuffer,
            const ScreenLayoutData& screenLayout
        ) noexcept;

        PixelBuffer buffer;
        // Used as a staging area for the hybrid screen to be scaled
        PixelBuffer hybridBuffer;
        retro::Scaler hybridScaler;
    };
}

#endif // MELONDSDS_RENDER_SOFTWARE_HPP
