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

#include <cstdarg>
#include <libretro.h>
#include <retro_timers.h>
#include <Platform.h>
#include <compat/strl.h>
#include "../memory.hpp"
#include "../environment.hpp"
#include "../config.hpp"

void Platform::Init(int, char **) {
    // these args are not used in libretro
    retro::log(RETRO_LOG_DEBUG, "Platform::Init\n");
}

void Platform::DeInit() {
    retro::log(RETRO_LOG_DEBUG, "Platform::DeInit\n");
    melonds::NdsSaveManager.reset();
    melonds::GbaSaveManager.reset();
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