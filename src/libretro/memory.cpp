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

#include <cstring>
#include <NDS.h>
#include <NDSCart.h>
#include <ARCodeFile.h>
#include <AREngine.h>
#include <frontend/qt_sdl/Config.h>
#include "libretro.hpp"
#include "environment.hpp"
#include "config.hpp"
#include <retro_assert.h>

constexpr size_t DS_MEMORY_SIZE = 0x400000;
constexpr size_t DSI_MEMORY_SIZE = 0x1000000;
constexpr ssize_t SAVESTATE_SIZE_UNKNOWN = -1;

namespace AREngine {
    extern void RunCheat(ARCode &arcode);
}

namespace melonds {
    static ssize_t _savestate_size = SAVESTATE_SIZE_UNKNOWN;
}

static const char *const memory_type_name(unsigned type)
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
        default:
            return "<unknown>";
    }
}

/// Savestates in melonDS can vary in size depending on the game,
/// so we have to try saving the state first before we can know how big it'll be.
PUBLIC_SYMBOL size_t retro_serialize_size(void) {
    if (melonds::_savestate_size < 0) {
        // If we haven't yet figured out how big the savestate should be...

        if (Config::ConsoleType == melonds::ConsoleType::DSi) {
            // DSi mode doesn't support savestates right now
            melonds::_savestate_size = 0;
            // TODO: When DSi mode supports savestates, remove this conditional block
        } else {
            Savestate state;
            NDS::DoSavestate(&state);
            melonds::_savestate_size = state.Length();

            retro::log(
                RETRO_LOG_INFO,
                "Savestate requires %dB = %.0fKiB = %.0fMiB (before compression)",
                melonds::_savestate_size,
                melonds::_savestate_size / 1024.0f,
                melonds::_savestate_size / 1024.0f / 1024.0f
            );
        }
    }

    return melonds::_savestate_size;
}

PUBLIC_SYMBOL bool retro_serialize(void *data, size_t size) {
    memset(data, 0, size);

    Savestate state(data, size, true);

    return NDS::DoSavestate(&state) && !state.Error;
}

PUBLIC_SYMBOL bool retro_unserialize(const void *data, size_t size) {
    retro::log(RETRO_LOG_DEBUG, "retro_unserialize(%p, %d)", data, size);

    Savestate savestate((u8 *) data, size, false);

    return NDS::DoSavestate(&savestate) && !savestate.Error;
}

PUBLIC_SYMBOL void *retro_get_memory_data(unsigned type) {
    retro::log(RETRO_LOG_DEBUG, "retro_get_memory_data(%s)\n", memory_type_name(type));
    switch (type) {
        case RETRO_MEMORY_SYSTEM_RAM:
            return NDS::MainRAM;
        case RETRO_MEMORY_SAVE_RAM:
            return NDSCart::GetSaveMemory();
        default:
            return nullptr;
    }
}

PUBLIC_SYMBOL size_t retro_get_memory_size(unsigned type) {
    switch (type) {
        case RETRO_MEMORY_SYSTEM_RAM:
            switch (Config::ConsoleType) {
                default:
                    retro::log(RETRO_LOG_WARN,
                               "Unknown console type %d, returning memory size of 4MB (as used by the DS).",
                               Config::ConsoleType);
                    // Intentional fall-through
                case melonds::ConsoleType::DS:
                    return DS_MEMORY_SIZE; // 4MB, the size of the DS system RAM
                case melonds::ConsoleType::DSi:
                    return DSI_MEMORY_SIZE; // 16MB, the size of the DSi system RAM
            }
        case RETRO_MEMORY_SAVE_RAM:
            return NDSCart::GetSaveMemoryLength();
        default:
            return 0;
    }
}

PUBLIC_SYMBOL void retro_cheat_reset(void) {}

PUBLIC_SYMBOL void retro_cheat_set(unsigned index, bool enabled, const char *code) {
    if (!enabled)
        return;
    ARCode curcode;
    std::string str(code);
    char *pch = &*str.begin();
    curcode.Name = code;
    curcode.Enabled = enabled;
    curcode.CodeLen = 0;
    pch = strtok(pch, " +");
    while (pch != nullptr) {
        curcode.Code[curcode.CodeLen] = (u32) strtol(pch, nullptr, 16);
        retro::log(RETRO_LOG_INFO, "Adding Code %s (%d) \n", pch, curcode.Code[curcode.CodeLen]);
        curcode.CodeLen++;
        pch = strtok(nullptr, " +");
    }
    AREngine::RunCheat(curcode);
}

// TODO: See NDS.cpp for details on the memory maps (see ARM9Read8)
bool melonds::set_memory_descriptors() {
    retro_memory_descriptor descriptors[] = {
        {
            .flags = RETRO_MEMDESC_SYSTEM_RAM,
            .ptr = NDS::MainRAM,
            .start = 0x02000000,
            .select = 0,
            .disconnect = Config::ConsoleType == DSi ? ~(size_t) 0xFFFFFF : ~(size_t) 0x3FFFFF,
            .len = Config::ConsoleType == DSi ? DSI_MEMORY_SIZE : DS_MEMORY_SIZE,
            .addrspace = "RAM",
        },
        {
            .flags = RETRO_MEMDESC_SAVE_RAM,
            .ptr = NDSCart::GetSaveMemory(),
            .start = 0,
            .select = 0,
            .disconnect = 0,
            .len = NDSCart::GetSaveMemoryLength(),
            .addrspace = "SRAM",
        },
    };

    retro_memory_map memory_map{
        .descriptors = descriptors,
        .num_descriptors = sizeof(descriptors) / sizeof(descriptors[0]),
    };

    return retro::environment(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &memory_map);
}

void melonds::clear_memory_config() {
    _savestate_size = SAVESTATE_SIZE_UNKNOWN;
}