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
#include <AREngine.h>
#include "libretro.hpp"
#include "environment.hpp"
#include "config.hpp"
#include "info.hpp"
#include <retro_assert.h>
#include "content.hpp"
#include "core.hpp"

#include "tracy.hpp"
#include "sram.hpp"
#include "format.hpp"

using std::byte;
using std::span;

using namespace melonDS;
constexpr size_t DS_MEMORY_SIZE = 0x400000;
constexpr size_t DSI_MEMORY_SIZE = 0x1000000;
constexpr ssize_t SAVESTATE_SIZE_UNKNOWN = -1;

namespace MelonDsDs {
    static ssize_t _savestate_size = SAVESTATE_SIZE_UNKNOWN;
}

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
    retro::debug("retro_get_memory_data({})\n", memory_type_name(type));
    if (MelonDsDs::IsInErrorScreen())
        return nullptr;
    switch (type) {
        case RETRO_MEMORY_SYSTEM_RAM:
            retro_assert(MelonDsDs::Core.Console != nullptr);
            return MelonDsDs::Core.Console->MainRAM;
        case RETRO_MEMORY_SAVE_RAM:
            if (MelonDsDs::sram::NdsSaveManager) {
                return MelonDsDs::sram::NdsSaveManager->Sram();
            }
            [[fallthrough]];
        default:
            return nullptr;
    }
}

PUBLIC_SYMBOL size_t retro_get_memory_size(unsigned type) {
    using namespace MelonDsDs;
    if (MelonDsDs::IsInErrorScreen())
        return 0;
    ConsoleType console_type = config::system::ConsoleType();
    switch (type) {
        case RETRO_MEMORY_SYSTEM_RAM:
            switch (console_type) {
                default:
                    retro::warn("Unknown console type {}, returning memory size of 4MB (as used by the DS).", console_type);
                    // Intentional fall-through
                case MelonDsDs::ConsoleType::DS:
                    return DS_MEMORY_SIZE; // 4MB, the size of the DS system RAM
                case MelonDsDs::ConsoleType::DSi:
                    return DSI_MEMORY_SIZE; // 16MB, the size of the DSi system RAM
            }
        case RETRO_MEMORY_SAVE_RAM:
            if (MelonDsDs::sram::NdsSaveManager) {
                return MelonDsDs::sram::NdsSaveManager->SramLength();
            }
            [[fallthrough]];
        default:
            return 0;
    }
}

void MelonDsDs::clear_memory_config() {
    _savestate_size = SAVESTATE_SIZE_UNKNOWN;
}