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
#include <cstring>
#include <regex>
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

/// Savestates in melonDS can vary in size depending on the game,
/// so we have to try saving the state first before we can know how big it'll be.
/// RetroArch may try to call this function before the ROM is installed
/// if rewind mode is enabled
PUBLIC_SYMBOL size_t retro_serialize_size(void) {
    ZoneScopedN("retro_serialize_size");
    if (MelonDsDs::IsInErrorScreen())
        return 0;

    using namespace MelonDsDs;
    if (MelonDsDs::_savestate_size < 0) {
        // If we haven't yet figured out how big the savestate should be...

        if (config::system::ConsoleType() == ConsoleType::DSi) {
            // DSi mode doesn't support savestates right now
            MelonDsDs::_savestate_size = 0;
            // TODO: When DSi mode supports savestates, remove this conditional block
        } else {
            retro_assert(MelonDsDs::Core.Console != nullptr);
            NDS& nds = *MelonDsDs::Core.Console;
            #ifndef NDEBUG
            if (retro::content::get_loaded_nds_info() != std::nullopt) {
                // If we're booting with a ROM...

                // Savestate size varies by several factors, but SRAM length is the big one.
                // We won't know the size of the cart's SRAM until it's loaded,
                // so we can't know the savestate size until then.
                // We must ensure the cart is loaded before the frontend starts to ask about the savestate size!
                retro_assert(nds.NDSCartSlot.GetCart() != nullptr);
            }
            #endif

            Savestate state;
            nds.DoSavestate(&state);
            MelonDsDs::_savestate_size = state.Length();

            retro::info(
                "Savestate requires {}B = {}KiB = {}MiB (before compression)",
                MelonDsDs::_savestate_size,
                MelonDsDs::_savestate_size / 1024.0f,
                MelonDsDs::_savestate_size / 1024.0f / 1024.0f
            );
        }
    }

    return MelonDsDs::_savestate_size;
}

PUBLIC_SYMBOL bool retro_serialize(void *data, size_t size) {
    ZoneScopedN("retro_serialize");
    if (MelonDsDs::IsInErrorScreen())
        return false;

    retro_assert(MelonDsDs::Core.Console != nullptr);
    NDS& nds = *MelonDsDs::Core.Console;
#ifndef NDEBUG
    if (retro::content::get_loaded_nds_info() != std::nullopt) {
        // If we're booting with a ROM...
        retro_assert(nds.NDSCartSlot.GetCart() != nullptr);
    }
#endif
    retro_assert(size == MelonDsDs::_savestate_size);

    Savestate state(data, size, true);

    return nds.DoSavestate(&state) && !state.Error;
}

PUBLIC_SYMBOL bool retro_unserialize(const void *data, size_t size) {
    ZoneScopedN("retro_unserialize");
    retro::debug("retro_unserialize({}, {})", data, size);
    if (MelonDsDs::IsInErrorScreen())
        return false;

    retro_assert(MelonDsDs::Core.Console != nullptr);
    NDS& nds = *MelonDsDs::Core.Console;

#ifndef NDEBUG
    if (retro::content::get_loaded_nds_info() != std::nullopt) {
        // If we're booting with a ROM...
        retro_assert(nds.NDSCartSlot.GetCart() != nullptr);
    }
#endif

    Savestate savestate((u8 *) data, size, false);

    if (savestate.Error) {
        u16 major = savestate.MajorVersion();
        u16 minor = savestate.MinorVersion();
        retro::error("Expected a savestate of major version {}, got {}.{}", SAVESTATE_MAJOR, major, minor);

        if (major < SAVESTATE_MAJOR) {
            // If this savestate is too old...
            retro::set_error_message(
                "This savestate is too old, can't load it.\n"
                "Save your game normally in the older version and import the save data.");
        } else if (major > SAVESTATE_MAJOR) {
            // If this savestate is too new...
            retro::set_error_message(
                "This savestate too new, can't load it.\n"
                "Save your game normally in the newer version, "
                "then update this core or import the save data.");
        }

        return false;
    }

    if (size != MelonDsDs::_savestate_size) {
        retro::error("Expected a {}-byte savestate, got one of {} bytes", MelonDsDs::_savestate_size, size);
        retro::set_error_message("Can't load this savestate, most likely the ROM or the core is wrong.");
        return false;
    }

    return nds.DoSavestate(&savestate) && !savestate.Error;
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