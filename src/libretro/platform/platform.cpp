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

#include <array>
#include <cstdarg>

#include <gfx/scaler/scaler.h>
#include <libretro.h>
#include <retro_timers.h>
#include <Platform.h>
#include <compat/strl.h>
#include <retro_assert.h>
#include <retro_endianness.h>

#include "../memory.hpp"
#include "../environment.hpp"
#include "../config.hpp"
#include "sram.hpp"

constexpr unsigned DSI_CAMERA_WIDTH = 640;
constexpr unsigned DSI_CAMERA_HEIGHT = 480;
static struct retro_camera_callback _camera{};
static struct scaler_ctx _scaler{};

// YUV422 framebuffer, two pixels per 32-bit int (see DSi_Camera.h)
static std::array<uint32_t, DSI_CAMERA_WIDTH * DSI_CAMERA_HEIGHT / 2> _camera_buffer{};

#define YUV_SHIFT 6
#define YUV_OFFSET (1 << (YUV_SHIFT - 1))
#define YUV_MAT_Y (1 << 6)
#define YUV_MAT_U_G (-22)
#define YUV_MAT_U_B (113)
#define YUV_MAT_V_R (90)
#define YUV_MAT_V_G (-46)

static void conv_argb8888_yuyv(void *output_, const void *input_,
                               int width, int height,
                               int out_stride, int in_stride) {
    const uint32_t* src = static_cast<const uint32_t *>(input_);
    uint32_t* dst = static_cast<uint32_t *>(output_);
    for (int dy = 0; dy < height; dy++)
    {
        int sy = (dy * height) / height;

        for (int dx = 0; dx < width; dx+=2)
        {
            int sx;

            sx = (dx * width) / width;

            u32 pixel1 = SWAP32(src[sy*width + sx]);

            sx = ((dx+1) * width) / width;

            u32 pixel2 = SWAP32(src[sy*width + sx]);

            int r1 = (pixel1 >> 16) & 0xFF;
            int g1 = (pixel1 >> 8) & 0xFF;
            int b1 = pixel1 & 0xFF;

            int r2 = (pixel2 >> 16) & 0xFF;
            int g2 = (pixel2 >> 8) & 0xFF;
            int b2 = pixel2 & 0xFF;

            int y1 = ((r1 * 19595) + (g1 * 38470) + (b1 * 7471)) >> 16;
            int u1 = ((b1 - y1) * 32244) >> 16;
            int v1 = ((r1 - y1) * 57475) >> 16;

            int y2 = ((r2 * 19595) + (g2 * 38470) + (b2 * 7471)) >> 16;
            int u2 = ((b2 - y2) * 32244) >> 16;
            int v2 = ((r2 - y2) * 57475) >> 16;

            u1 += 128; v1 += 128;
            u2 += 128; v2 += 128;

            y1 = std::clamp(y1, 0, 255); u1 = std::clamp(u1, 0, 255); v1 = std::clamp(v1, 0, 255);
            y2 = std::clamp(y2, 0, 255); u2 = std::clamp(u2, 0, 255); v2 = std::clamp(v2, 0, 255);

            // huh
            u1 = (u1 + u2) >> 1;
            v1 = (v1 + v2) >> 1;

            dst[(dy*width + dx) / 2] = y1 | (u1 << 8) | (y2 << 16) | (v1 << 24);
        }
    }
}

static void CaptureImage(const uint32_t *buffer, unsigned width, unsigned height, size_t pitch) noexcept {

    if (_scaler.in_width != static_cast<int>(width) || _scaler.in_height != static_cast<int>(height) ||
        _scaler.in_stride != static_cast<int>(pitch)) {
        // If the camera's dimensions changed behind our back (or haven't been initialized)...
        _scaler.in_width = width;
        _scaler.in_height = height;
        _scaler.in_stride = pitch;
        _scaler.out_width = DSI_CAMERA_WIDTH;
        _scaler.out_height = DSI_CAMERA_HEIGHT;
        _scaler.out_stride = DSI_CAMERA_WIDTH * sizeof(uint32_t) / 2;
        _scaler.direct_pixconv = conv_argb8888_yuyv;
        _scaler.in_pixconv = nullptr;
        _scaler.out_pixconv = conv_argb8888_yuyv;

        if (!scaler_ctx_gen_filter(&_scaler)) {
            retro::warn("Failed to initialize camera scaler.");
        }
    }

    scaler_ctx_scale(&_scaler, _camera_buffer.data(), buffer);
}

void Platform::Init(int, char **) {
    // these args are not used in libretro
    retro::log(RETRO_LOG_DEBUG, "Platform::Init\n");

    if (melonds::config::system::ConsoleType() != melonds::ConsoleType::DSi) {
        retro::info("Camera is only supported in DSi mode.\n");
        return;
    }

    _camera.caps = (1 << RETRO_CAMERA_BUFFER_RAW_FRAMEBUFFER);
    _camera.frame_raw_framebuffer = CaptureImage;
    _camera.frame_opengl_texture = nullptr;
    _camera.width = DSI_CAMERA_WIDTH;
    _camera.height = DSI_CAMERA_HEIGHT;

    _scaler.in_fmt = SCALER_FMT_ARGB8888;
    _scaler.out_fmt = SCALER_FMT_YUYV;
    _scaler.scaler_type = SCALER_TYPE_BILINEAR; // TODO: Make configurable

    if (!retro::environment(RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE, &_camera)) {
        retro::warn("Camera interface not available.");
    } else {
        retro::info("Got a camera with images of (%d x %d) pixels", _camera.width, _camera.height);
    }
}

// TODO: Call this in retro_unload_game and retro_reset
void Platform::DeInit() {
    retro::log(RETRO_LOG_DEBUG, "Platform::DeInit\n");

    if (_camera.stop) {
        _camera.stop();
    }

    _camera.start = nullptr;
    _camera.stop = nullptr;
    scaler_ctx_gen_reset(&_scaler);
}

void Platform::SignalStop(Platform::StopReason reason) {
    switch (reason) {
        case StopReason::BadExceptionRegion:
            retro::set_error_message("An internal error occurred in the emulated console.");
            break;
        case StopReason::GBAModeNotSupported:
            retro::set_error_message("GBA mode is not supported. Use a GBA core instead.");
            break;
        default:
            break;
            // no-op; not every stop reason needs a message shown to the user
    }
    retro::shutdown();
}

int Platform::InstanceID() {
    return 0;
}

std::string Platform::InstanceFileSuffix() {
    return "";
}

static retro_log_level to_retro_log_level(Platform::LogLevel level) {
    switch (level) {
        case Platform::LogLevel::Debug:
            return RETRO_LOG_DEBUG;
        case Platform::LogLevel::Info:
            return RETRO_LOG_INFO;
        case Platform::LogLevel::Warn:
            return RETRO_LOG_WARN;
        case Platform::LogLevel::Error:
            return RETRO_LOG_ERROR;
        default:
            return RETRO_LOG_WARN;
    }
}

void Platform::Log(Platform::LogLevel level, const char *fmt...) {
    retro_log_level retro_level = to_retro_log_level(level);
    va_list va;
    va_start(va, fmt);
    char text[1024] = "[melonDS] ";
    strlcat(text, fmt, sizeof(text));
    retro::vlog(retro_level, text, va);
    va_end(va);
}

/// This function exists to avoid causing potential recursion problems
/// with \c retro_sleep, as on Windows it delegates to a function named
/// \c Sleep.
static void sleep_impl(u64 usecs) {
    retro_sleep(usecs / 1000);
}

void Platform::Sleep(u64 usecs) {
    sleep_impl(usecs);
}

void Platform::WriteNDSSave(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen) {
    // TODO: Implement a Fast SRAM mode where the frontend is given direct access to the SRAM buffer
    if (melonds::sram::NdsSaveManager) {
        melonds::sram::NdsSaveManager->Flush(savedata, savelen, writeoffset, writelen);

        // No need to maintain a flush timer for NDS SRAM,
        // because retro_get_memory lets us delegate autosave to the frontend.
    }
}

void Platform::Camera_Start(int num) {
    if (_camera.start) {
        if (_camera.start()) {
            retro::debug("Started camera #%d", num);
        } else {
            retro::error("Failed to start camera #%d", num);
        }
    }
}

void Platform::Camera_Stop(int num) {
    if (_camera.stop) {
        _camera.stop();
        retro::debug("Stopped camera #%d", num);
    }
}

void Platform::Camera_CaptureFrame(int num, u32 *frame, int width, int height, bool yuv) {
    retro_assert(width == DSI_CAMERA_WIDTH);
    retro_assert(height == DSI_CAMERA_HEIGHT);

    memcpy(frame, _camera_buffer.data(), _camera_buffer.size() * sizeof(uint32_t));
}
