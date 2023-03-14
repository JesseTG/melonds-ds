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

#include <libretro.h>
#include <retro_timers.h>
#include <Platform.h>
#include "../environment.hpp"

namespace Platform {
    static int _instance_id;
}

void Platform::Init(int, char **) {
    // these args are not used in libretro
    retro::log(RETRO_LOG_DEBUG, "Platform::Init\n");

    // TODO: Initialize Platform resources
}

void Platform::DeInit() {
    retro::log(RETRO_LOG_DEBUG, "Platform::DeInit\n");
    _instance_id = 0;
    // TODO: Clean up Platform resources
}

void Platform::StopEmu() {
    retro::log(RETRO_LOG_DEBUG, "Platform::StopEmu\n");
}

int Platform::InstanceID() {
    return _instance_id;
}

// TODO: Split upstream implementation into a non-Qt file
std::string Platform::InstanceFileSuffix() {
    int instance = Platform::_instance_id;
    if (instance == 0) return "";

    char suffix[16] = {0};
    snprintf(suffix, 15, ".%d", instance + 1);
    return suffix;
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
    // TODO: Write data to disk
}
void Platform::WriteGBASave(const u8* savedata, u32 savelen, u32 writeoffset, u32 writelen)
{
    // TODO: Write data to disk
}