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

#include "core.hpp"

#include <libretro.h>
#include <retro_assert.h>

#include <NDS.h>

using std::byte;
constexpr size_t DS_MEMORY_SIZE = 0x400000;
constexpr size_t DSI_MEMORY_SIZE = 0x1000000;

MelonDsDs::CoreState MelonDsDs::Core(false);

MelonDsDs::CoreState::CoreState(bool init) noexcept : initialized(init) {
}

MelonDsDs::CoreState::~CoreState() noexcept {
    Console = nullptr;
    initialized = false;
}

retro_system_av_info MelonDsDs::CoreState::GetSystemAvInfo() const noexcept {
#ifndef NDEBUG
    if (!_messageScreen) {
        retro_assert(Console != nullptr);
    }
#endif

    return {
        .timing {
            .fps = 32.0f * 1024.0f * 1024.0f / 560190.0f,
            .sample_rate = 32.0f * 1024.0f,
        },
        .geometry = _screenLayout.Geometry(Console->GPU.GetRenderer3D()),
    };
}

/// Savestates in melonDS can vary in size depending on the game,
/// so we have to try saving the state first before we can know how big it'll be.
/// RetroArch may try to call this function before the ROM is installed
/// if rewind mode is enabled
size_t MelonDsDs::CoreState::SerializeSize() const noexcept {
    ZoneScopedN(TracyFunction);
    if (_messageScreen)
        return 0;
    // If there's an error, there's nothing to serialize

    if (!_savestateSize.has_value()) {
        // If we haven't yet figured out how big the savestate should be...

        retro_assert(Console != nullptr);
        if (static_cast<ConsoleType>(Console->ConsoleType) == ConsoleType::DSi) {
            // DSi mode doesn't support savestates right now
            _savestateSize = 0;
            // TODO: When DSi mode supports savestates, remove this conditional block
        }
        else {
#ifndef NDEBUG
            if (_ndsInfo) {
                // If we're booting with a ROM...

                // Savestate size varies by several factors, but SRAM length is the big one.
                // We won't know the size of the cart's SRAM until it's loaded,
                // so we can't know the savestate size until then.
                // We must ensure the cart is loaded before the frontend starts to ask about the savestate size!
                retro_assert(Console->NDSCartSlot.GetCart() != nullptr);
            }
#endif

            melonDS::Savestate state;
            Console->DoSavestate(&state);
            size_t length = state.Length();
            _savestateSize = length;

            retro::info(
                "Savestate requires {}B = {}KiB = {}MiB (before compression)",
                length,
                length / 1024.0f,
                length / 1024.0f / 1024.0f
            );
        }
    }

    retro_assert(_savestateSize.has_value());

    return *_savestateSize;
}

bool MelonDsDs::CoreState::Serialize(std::span<std::byte> data) const noexcept {
    ZoneScopedN(TracyFunction);
    if (_messageScreen)
        return false;

    retro_assert(Console != nullptr);

#ifndef NDEBUG
    if (_ndsInfo) {
        // If we're booting with a ROM...
        retro_assert(Console->GetNDSCart() != nullptr);
    }
#endif
    retro_assert(_savestateSize.has_value());
    retro_assert(data.size() == _savestateSize);

    melonDS::Savestate state(data.data(), data.size(), true);

    return Console->DoSavestate(&state) && !state.Error;
}

bool MelonDsDs::CoreState::Unserialize(std::span<const std::byte> data) noexcept {
    ZoneScopedN(TracyFunction);
    if (_messageScreen)
        return false;

    retro_assert(Console != nullptr);
    retro_assert(_savestateSize.has_value());

#ifndef NDEBUG
    if (_ndsInfo.has_value()) {
        // If we're booting with a ROM...
        retro_assert(Console->GetNDSCart() != nullptr);
    }
#endif

    melonDS::Savestate savestate(const_cast<void*>(static_cast<const void*>(data.data())), data.size(), false);

    if (savestate.Error) {
        uint16_t major = savestate.MajorVersion();
        uint16_t minor = savestate.MinorVersion();
        retro::error("Expected a savestate of major version {}, got {}.{}", SAVESTATE_MAJOR, major, minor);

        if (major < SAVESTATE_MAJOR) {
            // If this savestate is too old...
            retro::set_error_message(
                "This savestate is too old, can't load it.\n"
                "Save your game normally in the older version and import the save data.");
        }
        else if (major > SAVESTATE_MAJOR) {
            // If this savestate is too new...
            retro::set_error_message(
                "This savestate is too new, can't load it.\n"
                "Save your game normally in the newer version, "
                "then update this core or import the save data.");
        }

        return false;
    }

    if (data.size() != _savestateSize) {
        retro::error("Expected a {}-byte savestate, got one of {} bytes", _savestateSize, data.size());
        retro::set_error_message("Can't load this savestate, most likely the ROM or the core is wrong.");
        return false;
    }

    return Console->DoSavestate(&savestate) && !savestate.Error;
}

byte* MelonDsDs::CoreState::GetMemoryData(unsigned id) noexcept {
    ZoneScopedN(TracyFunction);
    if (_messageScreen)
        return nullptr;

    switch (id) {
        case RETRO_MEMORY_SYSTEM_RAM:
            retro_assert(Console != nullptr);
            return reinterpret_cast<byte*>(Console->MainRAM);
        case RETRO_MEMORY_SAVE_RAM:
            if (_ndsSaveManager) {
                return (byte*)_ndsSaveManager->Sram();
            }
            [[fallthrough]];
        default:
            return nullptr;
    }
}

size_t MelonDsDs::CoreState::GetMemorySize(unsigned id) noexcept {
    if (_messageScreen)
        return 0;

    switch (id) {
        case RETRO_MEMORY_SYSTEM_RAM: {
            retro_assert(Console != nullptr);
            auto consoleType = static_cast<ConsoleType>(Console->ConsoleType);
            switch (consoleType) {
                default:
                    retro::warn("Unknown console type {}, returning memory size of 4MB.", consoleType);
                    [[fallthrough]];
                // Intentional fall-through
                case ConsoleType::DS:
                    return DS_MEMORY_SIZE; // 4MB, the size of the DS system RAM
                case ConsoleType::DSi:
                    return melonDS::MainRAMMaxSize; // 16MB, the size of the DSi system RAM
            }
        }
        case RETRO_MEMORY_SAVE_RAM:
            if (_ndsSaveManager) {
                return _ndsSaveManager->SramLength();
            }
            [[fallthrough]];
        default:
            return 0;
    }
}
