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
#include "../memory.hpp"
#include "../environment.hpp"
#include "../config.hpp"

constexpr unsigned DSI_CAMERA_WIDTH = 640;
constexpr unsigned DSI_CAMERA_HEIGHT = 480;
static struct retro_camera_callback _camera {};
static struct scaler_ctx _scaler {};
static std::array<uint32_t, DSI_CAMERA_WIDTH * DSI_CAMERA_HEIGHT> _camera_buffer {};


static void CaptureImage(const uint32_t *buffer, unsigned width, unsigned height, size_t pitch) {
    // TODO: If the size and pitch are different, reinitialize the scaler

    scaler_ctx_scale(&_scaler, _camera_buffer.data(), buffer);
    if (width == DSI_CAMERA_WIDTH && height == DSI_CAMERA_HEIGHT) {
        memcpy(_camera_buffer.data(), buffer, sizeof(_camera_buffer));
    } else {
        // TODO: Scale the image to 640x480
    }
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

    // TODO: Configure the scaler
    _scaler.out_width = DSI_CAMERA_WIDTH;
    _scaler.out_height = DSI_CAMERA_HEIGHT;
    _scaler.in_fmt = SCALER_FMT_ARGB8888;
    _scaler.out_fmt = SCALER_FMT_YUYV;
    _scaler.scaler_type = SCALER_TYPE_BILINEAR; // TODO: Make configurable

    if (!retro::environment(RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE, &_camera)) {
        retro::warn("Camera interface not available.");
    }

    if (!scaler_ctx_gen_filter(&_scaler)) {
        retro::warn("Failed to initialize camera scaler.");
    }
}

// TODO: Call this in retro_unload_game and retro_reset
void Platform::DeInit() {
    retro::log(RETRO_LOG_DEBUG, "Platform::DeInit\n");
    melonds::NdsSaveManager.reset();
    melonds::GbaSaveManager.reset();

    if (_camera.stop) {
        _camera.stop();
    }

    _camera.start = nullptr;
    _camera.stop = nullptr;
    scaler_ctx_gen_reset(&_scaler);
}

void Platform::StopEmu() {
    retro::shutdown();
}

int Platform::InstanceID() {
    return 0;
}

std::string Platform::InstanceFileSuffix() {
    return "";
}

static retro_log_level to_retro_log_level(Platform::LogLevel level)
{
    switch (level)
    {
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

void Platform::Log(Platform::LogLevel level, const char* fmt...)
{
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

void Platform::WriteNDSSave(const u8* savedata, u32 savelen, u32 writeoffset, u32 writelen)
{
    // TODO: Implement a Fast SRAM mode where the frontend is given direct access to the SRAM buffer
    if (melonds::NdsSaveManager) {
        melonds::NdsSaveManager->Flush(savedata, savelen, writeoffset, writelen);

        // No need to maintain a flush timer for NDS SRAM,
        // because retro_get_memory lets us delegate autosave to the frontend.
    }
}

void Platform::WriteGBASave(const u8* savedata, u32 savelen, u32 writeoffset, u32 writelen)
{
    if (melonds::GbaSaveManager) {
        melonds::GbaSaveManager->Flush(savedata, savelen, writeoffset, writelen);

        // Start the countdown until we flush the SRAM back to disk.
        // The timer resets every time we write to SRAM,
        // so that a sequence of SRAM writes doesn't result in
        // a sequence of disk writes.
        melonds::TimeToGbaFlush = melonds::config::save::FlushDelay();
    }
}

void Platform::Camera_Start(int num) {
    if (_camera.start) {
        _camera.start();
    }
}

void Platform::Camera_Stop(int num) {
    if (_camera.stop) {
        _camera.stop();
    }
}

void Platform::Camera_CaptureFrame(int num, u32 *frame, int width, int height, bool yuv) {
    // TODO: Copy the image from _camera_buffer to frame
    // TODO: Convert the image to YUV if necessary
}
