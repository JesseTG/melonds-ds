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

#include "memory.hpp"

#include "PlatformOGLPrivate.h"
#include <cstddef>
#include <cstring>
#include <regex>
#include <span>

#include <fmt/core.h>
#include <NDS.h>
#include <NDSCart.h>
#include "libretro.hpp"
#include "environment.hpp"
#include "config.hpp"
#include "info.hpp"
#include "core.hpp"

#include "tracy.hpp"
#include "format.hpp"

using std::byte;
using std::span;

static const char *memory_type_name(unsigned type)
{
    switch (type) {
        case RETRO_MEMORY_SAVE_RAM:
            return "RETRO_MEMORY_SAVE_RAM";
        case RETRO_MEMORY_RTC:
            return "RETRO_MEMORY_RTC";
        case RETRO_MEMORY_SYSTEM_RAM:
            return "RETRO_MEMORY_SYSTEM_RAM";
        case RETRO_MEMORY_VIDEO_RAM:
            return "RETRO_MEMORY_VIDEO_RAM";
        case MelonDsDs::MELONDSDS_MEMORY_GBA_SAVE_RAM:
            return "MELONDSDS_MEMORY_GBA_SAVE_RAM";
        default:
            return "<unknown>";
    }
}

PUBLIC_SYMBOL size_t retro_serialize_size(void) {
    ZoneScopedN(TracyFunction);

    return MelonDsDs::Core.SerializeSize();
}

PUBLIC_SYMBOL bool retro_serialize(void *data, size_t size) {
    ZoneScopedN(TracyFunction);

    return MelonDsDs::Core.Serialize(std::span(static_cast<byte*>(data), size));
}

PUBLIC_SYMBOL bool retro_unserialize(const void *data, size_t size) {
    ZoneScopedN(TracyFunction);
    retro::debug("retro_unserialize({}, {})", data, size);

    return MelonDsDs::Core.Unserialize(std::span(const_cast<byte*>(static_cast<const byte*>(data)), size));
}

PUBLIC_SYMBOL void *retro_get_memory_data(unsigned type) {
    ZoneScopedN(TracyFunction);
    retro::debug("retro_get_memory_data({})\n", memory_type_name(type));

    return MelonDsDs::Core.GetMemoryData(type);
}

PUBLIC_SYMBOL size_t retro_get_memory_size(unsigned type) {
    ZoneScopedN(TracyFunction);

    return MelonDsDs::Core.GetMemorySize(type);
}