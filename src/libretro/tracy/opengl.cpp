/*
    Copyright 2024 Jesse Talavera

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
#include "screenlayout.hpp"

MelonDsDs::OpenGlTracyCapture::OpenGlTracyCapture() {
    if (!tracy::ProfilerAvailable()) {
        throw std::runtime_error("Tracy not available");
    }

    // We're going to send the OpenGL-rendered image to tracy, but for performance reasons:
    //   - We want to scale it down to the DS's native size (if necessary)
    //   - We want to do this asynchronously, so we don't block the CPU
    //   - The rendering can run ahead of the GPU by a few frames

    ZoneScopedN(TracyFunction);
    TracyGpuZone(TracyFunction);

    // Allocate the textures for the resized image
    glGenTextures(4, _tracyTextures.data());

    // Create some FBOs to let us write to the textures
    glGenFramebuffers(4, _tracyFbos.data());

    // Create some PBOs to let the CPU read from the textures
    glGenBuffers(4, _tracyPbos.data());
    for (int i = 0; i < 4; i++) {
        // Let's configure one texture at a time...
        glBindTexture(GL_TEXTURE_2D, _tracyTextures[i]);

        // We'll use nearest-neighbor interpolation to avoid blurring
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // And we want our texture to be 2D, in RGBA format, big enough to hold a pair of NDS screens without mipmaps,
        // and with each component being an unsigned byte.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT * 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Now we'll configure the FBO used to draw to this texture...
        glBindFramebuffer(GL_FRAMEBUFFER, _tracyFbos[i]);

        // ...we'll attach a texture to the new FBO.
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tracyTextures[i], 0);

        // And we'll create a new PBO so we can read from the texture.
        glBindBuffer(GL_PIXEL_PACK_BUFFER, _tracyPbos[i]);

        // And the PBO has to be big enough to hold two NDS screens.
        glBufferData(GL_PIXEL_PACK_BUFFER, NDS_SCREEN_AREA<GLuint> * 2 * 4, nullptr, GL_STREAM_READ);
    }

    retro::debug("Initialized OpenGL Tracy capture");
}

MelonDsDs::OpenGlTracyCapture::~OpenGlTracyCapture() noexcept {
    ZoneScopedN(TracyFunction);
    TracyGpuZone(TracyFunction);

    // Clean up the textures
    glDeleteTextures(4, _tracyTextures.data());

    // Clean up the FBOs
    glDeleteFramebuffers(4, _tracyFbos.data());

    // Clean up the PBOs
    glDeleteBuffers(4, _tracyPbos.data());

    // Clean up the fences
    for (int i = 0; i < 4; i++) {
        glDeleteSync(_tracyFences[i]);
    }
}

void MelonDsDs::OpenGlTracyCapture::CaptureFrame(float scale) noexcept {
    if (!tracy::ProfilerAvailable()) {
        return;
    }

    ZoneScopedN(TracyFunction);
    TracyGpuZone(TracyFunction);

    // TODO: Capture the OpenGL renderer's buffer, not the RetroArch framebuffer
    while (!_tracyQueue.empty()) {
        // Until we've checked all the capture fences...

        // Pull the oldest capture fence from the queue
        const auto fiIdx = _tracyQueue.front();

        // Check this fence, but don't wait for it
        // If the fence hasn't gone off yet, then stop checking
        // (none of the newer fences will have been signaled yet)
        if (glClientWaitSync(_tracyFences[fiIdx], 0, 0) == GL_TIMEOUT_EXPIRED) break;

        // The fence has been signaled!
        // That means the capture we want is ready to send to Tracy

        // Thanks for your hard work, fence; you're no longer needed
        glDeleteSync(_tracyFences[fiIdx]);

        // Get the capture PBO ready to read its contents out...
        glBindBuffer(GL_PIXEL_PACK_BUFFER, _tracyPbos[fiIdx]);

        // Expose the capture PBO's contents to RAM
        auto ptr = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, NDS_SCREEN_AREA<GLuint> * 2 * 4, GL_MAP_READ_BIT);

        // Send the frame to Tracy
        FrameImage(ptr, NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT * 2, _tracyQueue.size(), true);

        // We're done with the capture PBO
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        _tracyQueue.pop();
    }

    // TODO: Only downscale if playing at a scale factor other than 1
    assert(_tracyQueue.empty() || _tracyQueue.front() != _tracyIndex); // check for buffer overrun

    // Get the capture FBO ready to receive the screen(s)...
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _tracyFbos[_tracyIndex]);

    // Copy the active framebuffer's contents to the capture FBO, downscaling along the way
    glBlitFramebuffer(0, 0, NDS_SCREEN_WIDTH * scale, NDS_SCREEN_HEIGHT * 2 * scale, 0, 0, NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT * 2, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // Okay, we're done downscaling the screen
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // TODO: Use the retro_hw_render_callback's default FBO

    // Get the capture FBO ready to read its contents out...
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _tracyFbos[_tracyIndex]);

    // Get the PBO ready to receive the downscaled screen(s)...
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _tracyPbos[_tracyIndex]);

    // Actually read the screen into the PBO
    // (nullptr means to read data into the bound PBO, not to the CPU)
    glReadPixels(0, 0, NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT * 2, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Okay, now we're done with the capture FBO; you can have the default FBO back
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); // TODO: Use the retro_hw_render_callback's default FBO

    // Create a new fence that'll go off when every OpenGL command that came before it finishes
    // (No other acceptable arguments are currently defined for glFenceSync)
    _tracyFences[_tracyIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // "Hang onto this flag for now, we'll check it again next frame."
    _tracyQueue.push(_tracyIndex);
    _tracyIndex = (_tracyIndex + 1) % 4;
}