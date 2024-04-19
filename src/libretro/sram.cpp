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

#include "sram.hpp"

#include <cstring>
#include <memory>
#include <optional>
#include <string_view>

#include <file/file_path.h>
#include <retro_assert.h>
#include <streams/file_stream.h>
#include <streams/rzip_stream.h>

#include "PlatformOGLPrivate.h"
#include <NDS.h>
#include <Platform.h>
#include <SPI.h>
#include <string/stdstring.h>

#include "config/config.hpp"
#include "config/constants.hpp"
#include "core/core.hpp"
#include "environment.hpp"
#include "exceptions.hpp"
#include "libretro.hpp"
#include "retro/task_queue.hpp"
#include "tracy.hpp"

using std::optional;
using std::nullopt;
using std::unique_ptr;
using std::make_unique;
using std::string;
using std::string_view;
using namespace melonDS;

MelonDsDs::sram::SaveManager::SaveManager(u32 initialLength) :
    _sram(std::make_unique<u8[]>(initialLength)),
    _sram_length(initialLength) {
}

MelonDsDs::sram::SaveManager::SaveManager(SaveManager&& other) noexcept :
    _sram(std::move(other._sram)),
    _sram_length(other._sram_length) {
    other._sram = nullptr;
    other._sram_length = 0;
}

MelonDsDs::sram::SaveManager& MelonDsDs::sram::SaveManager::operator=(SaveManager&& other) noexcept {
    if (this != &other) {
        _sram = std::move(other._sram);
        _sram_length = other._sram_length;
        other._sram = nullptr;
        other._sram_length = 0;
    }
    return *this;
}


void MelonDsDs::sram::SaveManager::Flush(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen) {
    ZoneScopedN(TracyFunction);
    if (_sram_length != savelen) {
        // If we loaded a game with a different SRAM length...

        _sram_length = savelen;
        _sram = std::make_unique<u8[]>(_sram_length);

        memcpy(_sram.get(), savedata, _sram_length);
    } else {
        if ((writeoffset + writelen) > savelen) {
            // If the write goes past the end of the SRAM, we have to wrap around
            u32 len = savelen - writeoffset;
            memcpy(_sram.get() + writeoffset, savedata + writeoffset, len);
            len = writelen - len;
            if (len > savelen) len = savelen;
            memcpy(_sram.get(), savedata, len);
        } else {
            memcpy(_sram.get() + writeoffset, savedata + writeoffset, writelen);
        }
    }
}

// Does not load the NDS SRAM, since retro_get_memory is used for that.
// But it will allocate the SRAM buffer
void MelonDsDs::CoreState::InitNdsSave(const NdsCart &nds_cart) {
    ZoneScopedN(TracyFunction);
    using std::runtime_error;
    if (nds_cart.GetHeader().IsHomebrew()) {
        // If this is a homebrew ROM...

        // Homebrew is a special case, as it uses an SD card rather than SRAM.
        // No need to explicitly load or save homebrew SD card images;
        // the CartHomebrew class does that.
        if (Config.DldiFolderSync()) {
            // If we're syncing the homebrew SD card image to the host filesystem...
            if (!path_mkdir(Config.DldiFolderPath().data())) {
                // Create the directory. If that fails...
                // (note that an existing directory is not an error)
                throw runtime_error(fmt::format("Failed to create virtual SD card directory at {}", Config.DldiFolderPath()));
            }
        }
    }
    else {
        // Get the length of the ROM's SRAM, if any
        u32 sram_length = nds_cart.GetSaveMemoryLength();

        if (sram_length > 0) {
            _ndsSaveManager = std::make_optional<sram::SaveManager>(sram_length);
            retro::debug("Allocated {}-byte SRAM buffer for loaded NDS ROM.", sram_length);
        } else {
            retro::debug("Loaded NDS ROM does not use SRAM.");
        }
        // The actual SRAM file is installed later; it's loaded into the core via retro_get_memory_data,
        // and it's applied in the first frame of retro_run.
    }
}

void MelonDsDs::CoreState::WriteNdsSave(std::span<const std::byte> savedata, uint32_t writeoffset, uint32_t writelen) noexcept {
    // No need to maintain a flush timer for NDS SRAM,
    // because retro_get_memory lets us delegate autosave to the frontend.

    if (_ndsSaveManager) {
        _ndsSaveManager->Flush((const uint8_t*)savedata.data(), savedata.size(), writeoffset, writelen);
    }
}

void MelonDsDs::CoreState::WriteGbaSave(std::span<const std::byte> savedata, uint32_t writeoffset, uint32_t writelen) noexcept {
    ZoneScopedN(TracyFunction);

    retro_assert(_gbaSaveManager.has_value());
    _gbaSaveManager->Flush((const uint8_t*)savedata.data(), savedata.size(), writeoffset, writelen);

    // Start the countdown until we flush the SRAM back to disk.
    // The timer resets every time we write to SRAM,
    // so that a sequence of SRAM writes doesn't result in
    // a sequence of disk writes.
    _timeToGbaFlush = Config.FlushDelay();
}

void MelonDsDs::CoreState::WriteFirmware(const Firmware& firmware, uint32_t writeoffset, uint32_t writelen) noexcept {
    ZoneScopedN(TracyFunction);

    _timeToFirmwareFlush = Config.FlushDelay();
}


