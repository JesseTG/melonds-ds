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

#include <retro_assert.h>

#include <NDS.h>

#include "input.hpp"
#include "screenlayout.hpp"

using glm::ivec2;
using glm::mat3;
using glm::vec3;
using glm::uvec2;
using std::span;

MelonDsDs::SoftwareRenderState::SoftwareRenderState() noexcept :
    buffer(1, 1),
    hybridBuffer(1, 1),
    hybridScaler() {
}

// TODO: Consider using RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER
void MelonDsDs::SoftwareRenderState::Render(
    melonDS::NDS& nds,
    const InputState& inputState,
    const CoreConfig& config,
    const ScreenLayoutData& screenLayout
) noexcept {
    ZoneScopedN(TracyFunction);

    if (buffer.Size() != screenLayout.BufferSize() || !buffer) {
        buffer = PixelBuffer(screenLayout.BufferSize());
    }

    if (IsHybridLayout(screenLayout.Layout())) {
        uvec2 requiredHybridBufferSize = NDS_SCREEN_SIZE<unsigned> * screenLayout.HybridRatio();
        if (hybridBuffer.Size() != requiredHybridBufferSize) {
            hybridBuffer = PixelBuffer(requiredHybridBufferSize);
        }

        hybridBuffer = PixelBuffer(requiredHybridBufferSize);
        hybridScaler = retro::Scaler(
            SCALER_FMT_ARGB8888,
            SCALER_FMT_ARGB8888,
            config.ScreenFilter() == ScreenFilter::Nearest ? SCALER_TYPE_POINT : SCALER_TYPE_BILINEAR,
            NDS_SCREEN_WIDTH,
            NDS_SCREEN_HEIGHT,
            requiredHybridBufferSize.x,
            requiredHybridBufferSize.y
        );
    }

    const uint32_t* topScreenBuffer = nds.GPU.Framebuffer[nds.GPU.FrontBuffer][0].get();
    const uint32_t* bottomScreenBuffer = nds.GPU.Framebuffer[nds.GPU.FrontBuffer][1].get();
    CombineScreens(
        span<const uint32_t, NDS_SCREEN_AREA<size_t>>(topScreenBuffer, NDS_SCREEN_AREA<size_t>),
        span<const uint32_t, NDS_SCREEN_AREA<size_t>>(bottomScreenBuffer, NDS_SCREEN_AREA<size_t>),
        screenLayout
    );

    if (!nds.IsLidClosed() && inputState.CursorVisible()) {
        DrawCursor(inputState, config, screenLayout);
    }

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

void MelonDsDs::SoftwareRenderState::CopyScreen(const uint32_t* src, uvec2 destTranslation, ScreenLayout layout) noexcept {
    ZoneScopedN(TracyFunction);
    // Only used for software rendering

    // melonDS's software renderer draws each emulated screen to its own buffer,
    // and then the frontend combines them based on the current layout.
    // In the original buffer, all pixels are contiguous in memory.
    // If a screen doesn't need anything drawn to its side (such as blank space or another screen),
    // then we can just copy the entire screen at once.
    // But if a screen *does* need anything drawn on either side of it,
    // then its pixels can't all be contiguous in memory.
    // In that case, we have to copy each row of pixels individually to a different offset.
    if (LayoutSupportsDirectCopy(layout)) {
        buffer.CopyDirect(src, destTranslation);
    }
    else {
        // Not all of this screen's pixels will be contiguous in memory, so we have to copy them row by row
        buffer.CopyRows(src, destTranslation, NDS_SCREEN_SIZE<unsigned>);
    }
}

void MelonDsDs::SoftwareRenderState::DrawCursor(const InputState& input, const CoreConfig& config,
    const ScreenLayoutData& screenLayout
) noexcept {
    ZoneScopedN(TracyFunction);
    // Only used for software rendering
    assert(buffer);

    if (screenLayout.Layout() == ScreenLayout::TopOnly)
        return;

    ivec2 cursorSize = ivec2(config.CursorSize());
    ivec2 clampedTouch = clamp(input.TouchPosition(), ivec2(0), ivec2(NDS_SCREEN_WIDTH - 1, NDS_SCREEN_HEIGHT - 1));
    ivec2 transformedTouch = screenLayout.GetBottomScreenMatrix() * vec3(clampedTouch, 1);

    uvec2 start = clamp(transformedTouch - ivec2(cursorSize), ivec2(0), ivec2(buffer.Size()));
    uvec2 end = clamp(transformedTouch + ivec2(cursorSize), ivec2(0), ivec2(buffer.Size()));

    for (uint32_t y = start.y; y < end.y; y++) {
        for (uint32_t x = start.x; x < end.x; x++) {
            // TODO: Replace with SIMD (does GLM have a SIMD version of this?)
            uint32_t& pixel = buffer[uvec2(x, y)];
            pixel = (0xFFFFFF - pixel) | 0xFF000000;
        }
    }
}

void MelonDsDs::SoftwareRenderState::CombineScreens(
    std::span<const uint32_t, NDS_SCREEN_AREA<size_t>> topBuffer,
    std::span<const uint32_t, NDS_SCREEN_AREA<size_t>> bottomBuffer,
    const ScreenLayoutData& screenLayout
) noexcept {
    ZoneScopedN(TracyFunction);

    buffer.Clear();
    ScreenLayout layout = screenLayout.Layout();

    if (IsHybridLayout(layout)) {
        retro_assert(hybridBuffer);
        auto primaryBuffer = layout == ScreenLayout::HybridTop ? topBuffer : bottomBuffer;

        hybridScaler.Scale(hybridBuffer[0], primaryBuffer.data());
        buffer.CopyRows(
            hybridBuffer[0],
            screenLayout.GetHybridScreenTranslation(),
            NDS_SCREEN_SIZE<unsigned> * screenLayout.HybridRatio()
        );

        HybridSideScreenDisplay smallScreenLayout = screenLayout.HybridSmallScreenLayout();

        if (smallScreenLayout == HybridSideScreenDisplay::Both || layout == ScreenLayout::HybridBottom) {
            // If we should display both screens, or if the bottom one is the primary...
            buffer.CopyRows(topBuffer.data(), screenLayout.GetTopScreenTranslation(), NDS_SCREEN_SIZE<unsigned>);
        }

        if (smallScreenLayout == HybridSideScreenDisplay::Both || layout == ScreenLayout::HybridTop) {
            // If we should display both screens, or if the top one is being focused...
            buffer.CopyRows(bottomBuffer.data(), screenLayout.GetBottomScreenTranslation(), NDS_SCREEN_SIZE<unsigned>);
        }
    }
    else {
        if (layout != ScreenLayout::BottomOnly)
            CopyScreen(topBuffer.data(), screenLayout.GetTopScreenTranslation(), layout);

        if (layout != ScreenLayout::TopOnly)
            CopyScreen(bottomBuffer.data(), screenLayout.GetBottomScreenTranslation(), layout);
    }
}

