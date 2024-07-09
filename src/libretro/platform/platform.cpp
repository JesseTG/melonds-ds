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
#include <cstdio>
#include <memory>

#include <gfx/scaler/scaler.h>
#include <libretro.h>
#include <retro_timers.h>
#include <Platform.h>
#include <compat/strl.h>
#include <retro_assert.h>
#include <retro_endianness.h>

#include "../environment.hpp"
#include "../config/config.hpp"
#include "../format.hpp"
#include "retro/scaler.hpp"
#include "sram.hpp"
#include "tracy.hpp"

using namespace melonDS;
constexpr unsigned DSI_CAMERA_WIDTH = 640;
constexpr unsigned DSI_CAMERA_HEIGHT = 480;

void Platform::SignalStop(Platform::StopReason reason, void* userdata) {
    retro::debug("Platform::SignalStop({})\n", reason);
    switch (reason) {
        case StopReason::BadExceptionRegion:
            retro::set_error_message("An internal error occurred in the emulated console.");
            retro::shutdown();
            break;
        case StopReason::GBAModeNotSupported:
            retro::set_error_message("GBA mode is not supported. Use a GBA core instead.");
            retro::shutdown();
            break;
        case StopReason::PowerOff:
            retro::shutdown();
            break;
        default:
            break;
            // no-op; not every stop reason needs a message shown to the user
    }
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

void Platform::WriteDateTime(int year, int month, int day, int hour, int minute, int second, void* userdata) {
    ZoneScopedN(TracyFunction);
    retro::debug("Platform::WriteDateTime({}, {}, {}, {}, {}, {})", year, month, day, hour, minute, second);
}

void Platform::Camera_Start(int num, void* userdata) {
}

void Platform::Camera_Stop(int num, void* userdata) {
}

void Platform::Camera_CaptureFrame(int num, u32 *frame, int width, int height, bool yuv, void* userdata) {
}
