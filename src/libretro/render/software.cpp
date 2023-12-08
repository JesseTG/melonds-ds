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

#include "software.hpp"
#include "input.hpp"
#include "screenlayout.hpp"

#include <NDS.h>

// TODO: Consider using RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER
void MelonDsDs::SoftwareRenderState::Render(melonDS::NDS& nds, const InputState& inputState, const CoreConfig& config, ScreenLayoutData& screenLayout) noexcept {
    ZoneScopedN(TracyFunction);

    const uint32_t* topScreenBuffer = nds.GPU.Framebuffer[nds.GPU.FrontBuffer][0].get();
    const uint32_t* bottomScreenBuffer = nds.GPU.Framebuffer[nds.GPU.FrontBuffer][1].get();
    screenLayout.CombineScreens(topScreenBuffer, bottomScreenBuffer);

    if (!nds.IsLidClosed() && inputState.CursorVisible()) {
        screenLayout.DrawCursor(config.CursorSize(), inputState.TouchPosition());
    }

    auto& buffer = screenLayout.Buffer();
    retro::video_refresh(buffer[0], buffer.Width(), buffer.Height(), buffer.Stride());

#ifdef HAVE_TRACY
    if (tracy::ProfilerAvailable()) {
        // If Tracy is connected...
        ZoneScopedN("MelonDsDs::render::RenderSoftware::SendFrameToTracy");
        std::unique_ptr<u8 []> frame = std::make_unique<u8[]>(buffer.Width() * buffer.Height() * 4);
        {
            ZoneScopedN("conv_argb8888_abgr8888");
            conv_argb8888_abgr8888(frame.get(), buffer[0], buffer.Width(), buffer.Height(), buffer.Stride(), buffer.Stride());
        }
        // libretro wants pixels in XRGB8888 format,
        // but Tracy wants them in XBGR8888 format.

        FrameImage(frame.get(), buffer.Width(), buffer.Height(), 0, false);
    }
#endif
}