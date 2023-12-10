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

#include "config.hpp"
#include "config/constants.hpp"
#include "core.hpp"
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
        _sram(new u8[initialLength]),
        _sram_length(initialLength) {
}

MelonDsDs::sram::SaveManager::~SaveManager() {
    delete[] _sram; // deleting null pointers is a no-op, no need to check
}

void MelonDsDs::sram::SaveManager::Flush(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen) {
    ZoneScopedN("MelonDsDs::sram::SaveManager::Flush");
    if (_sram_length != savelen) {
        // If we loaded a game with a different SRAM length...

        delete[] _sram;

        _sram_length = savelen;
        _sram = new u8[_sram_length];

        memcpy(_sram, savedata, _sram_length);
    } else {
        if ((writeoffset + writelen) > savelen) {
            // If the write goes past the end of the SRAM, we have to wrap around
            u32 len = savelen - writeoffset;
            memcpy(_sram + writeoffset, savedata + writeoffset, len);
            len = writelen - len;
            if (len > savelen) len = savelen;
            memcpy(_sram, savedata, len);
        } else {
            memcpy(_sram + writeoffset, savedata + writeoffset, writelen);
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
                throw runtime_error("Failed to create virtual SD card directory at " + Config.DldiFolderPath());
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

// Loads the GBA SRAM
void MelonDsDs::CoreState::InitGbaSram(GbaCart& gbaCart, const retro::GameInfo& gbaSaveInfo) {
    ZoneScopedN("MelonDsDs::sram::InitGbaSram");
    // We load the GBA SRAM file ourselves (rather than letting the frontend do it)
    // because we'll overwrite it later and don't want the frontend to hold open any file handles.
    // Due to libretro limitations, we can't use retro_get_memory_data to load the GBA SRAM
    // without asking the user to move their SRAM into the melonDS DS save folder.
    if (path_contains_compressed_file(gbaSaveInfo.GetPath().data())) {
        // If this save file is in an archive (e.g. /path/to/file.7z#mygame.srm)...

        // We don't support GBA SRAM files in archives right now;
        // libretro-common has APIs for extracting and re-inserting them,
        // but I just can't be bothered.
        retro::set_error_message(
                "melonDS DS does not support archived GBA save data right now. "
                "Please extract it and try again. "
                "Continuing without using the save data."
        );

        return;
    }

    // rzipstream opens the file as-is if it's not rzip-formatted
    rzipstream_t* gba_save_file = rzipstream_open(gbaSaveInfo.GetPath().data(), RETRO_VFS_FILE_ACCESS_READ);
    if (!gba_save_file) {
        throw std::runtime_error("Failed to open GBA save file");
    }

    if (rzipstream_is_compressed(gba_save_file)) {
        // If this save data is compressed in libretro's rzip format...
        // (not to be confused with a standard archive format like zip or 7z)

        // We don't support rzip-compressed GBA save files right now;
        // I can't be bothered.
        retro::set_error_message(
                "melonDS DS does not support compressed GBA save data right now. "
                "Please disable save data compression in the frontend and try again. "
                "Continuing without using the save data."
        );

        rzipstream_close(gba_save_file);
        return;
    }

    int64_t gba_save_file_size = rzipstream_get_size(gba_save_file);
    if (gba_save_file_size < 0) {
        // If we couldn't get the uncompressed size of the GBA save file...
        rzipstream_close(gba_save_file);
        throw std::runtime_error("Failed to get GBA save file size");
    }

    void* gba_save_data = malloc(gba_save_file_size);
    if (!gba_save_data) {
        rzipstream_close(gba_save_file);
        throw std::runtime_error("Failed to allocate memory for GBA save file");
    }

    if (rzipstream_read(gba_save_file, gba_save_data, gba_save_file_size) != gba_save_file_size) {
        rzipstream_close(gba_save_file);
        free(gba_save_data);
        throw std::runtime_error("Failed to read GBA save file");
    }

    _gbaSaveManager = std::make_optional<sram::SaveManager>(gba_save_file_size);
    gbaCart.SetSaveMemory(static_cast<const u8*>(gba_save_data), gba_save_file_size);
    // LoadSave's subclasses will call Platform::WriteGBASave.
    // The data will be in the buffer soon enough.
    retro::debug("Allocated {}-byte GBA SRAM", gbaCart.GetSaveMemoryLength());
    // Actually installing the SRAM will be done later, after NDS::Reset is called
    free(gba_save_data);
    rzipstream_close(gba_save_file);
    retro::task::push(sram::FlushGbaSramTask(gbaSaveInfo));
}

void Platform::WriteNDSSave(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen) {
    // TODO: Implement a Fast SRAM mode where the frontend is given direct access to the SRAM buffer
    ZoneScopedN(TracyFunction);
    if (MelonDsDs::sram::NdsSaveManager) {
        MelonDsDs::sram::NdsSaveManager->Flush(savedata, savelen, writeoffset, writelen);

        // No need to maintain a flush timer for NDS SRAM,
        // because retro_get_memory lets us delegate autosave to the frontend.
    }
}

void Platform::WriteGBASave(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen) {
    ZoneScopedN("Platform::WriteGBASave");
    if (MelonDsDs::sram::GbaSaveManager) {
        MelonDsDs::sram::GbaSaveManager->Flush(savedata, savelen, writeoffset, writelen);

        // Start the countdown until we flush the SRAM back to disk.
        // The timer resets every time we write to SRAM,
        // so that a sequence of SRAM writes doesn't result in
        // a sequence of disk writes.
        TimeToGbaFlush = MelonDsDs::config::save::FlushDelay();
    }
}

void Platform::WriteFirmware(const Firmware& firmware, u32 writeoffset, u32 writelen) {
    ZoneScopedN("Platform::WriteFirmware");

    TimeToFirmwareFlush = MelonDsDs::config::save::FlushDelay();
}


