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

#include <NDS.h>
#include <Platform.h>
#include <SPI.h>

#include "config.hpp"
#include "config/constants.hpp"
#include "content.hpp"
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

unique_ptr<melonds::sram::SaveManager> melonds::sram::NdsSaveManager;
unique_ptr<melonds::sram::SaveManager> melonds::sram::GbaSaveManager;
static optional<int> TimeToGbaFlush = nullopt;
static optional<int> TimeToFirmwareFlush = nullopt;

void melonds::sram::init() {
    ZoneScopedN("melonds::sram::init");
    retro_assert(NdsSaveManager == nullptr);
    retro_assert(GbaSaveManager == nullptr);
    TimeToGbaFlush = nullopt;
    TimeToFirmwareFlush = nullopt;
}

void melonds::sram::deinit() noexcept {
    ZoneScopedN("melonds::sram::deinit");
    NdsSaveManager = nullptr;
    GbaSaveManager = nullptr;
}

melonds::sram::SaveManager::SaveManager(u32 initialLength) :
        _sram(new u8[initialLength]),
        _sram_length(initialLength) {
}

melonds::sram::SaveManager::~SaveManager() {
    delete[] _sram; // deleting null pointers is a no-op, no need to check
}

void melonds::sram::SaveManager::Flush(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen) {
    ZoneScopedN("melonds::sram::SaveManager::Flush");
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

static void FlushGbaSram(retro::task::TaskHandle &task, const retro_game_info& gba_save_info) noexcept {
    ZoneScopedN("melonds::sram::FlushSram");
    using namespace melonds;
    const char* save_data_path = gba_save_info.path;
    if (save_data_path == nullptr || sram::GbaSaveManager == nullptr) {
        // No save data path was provided, or the GBA save manager isn't initialized
        return; // TODO: Report this error
    }
    const u8* gba_sram = sram::GbaSaveManager->Sram();
    u32 gba_sram_length = sram::GbaSaveManager->SramLength();

    if (gba_sram == nullptr || gba_sram_length == 0) {
        return; // TODO: Report this error
    }

    if (!filestream_write_file(save_data_path, gba_sram, gba_sram_length)) {
        retro::error("Failed to write %u-byte GBA SRAM to \"%s\"", gba_sram_length, save_data_path);
        // TODO: Report this to the user
    } else {
        retro::debug("Flushed %u-byte GBA SRAM to \"%s\"", gba_sram_length, save_data_path);
    }
}

static void FlushFirmware(string_view firmwarePath) noexcept {
    ZoneScopedN("melonds::sram::FlushFirmware");
    using SPI_Firmware::Firmware;
    using namespace melonds;

    retro_assert(!firmwarePath.empty() && firmwarePath.find('\0'));

    const Firmware* firmware = SPI_Firmware::GetFirmware();

    if (firmware == nullptr || firmware->Buffer() == 0) {
        retro::error("No firmware loaded in memory, cannot write to file");
        return;
    }

    // TODO: mimic melonds's behaviors

    if (!filestream_write_file(firmwarePath.data(), firmware->Buffer(), firmware->Length())) {
        retro::error("Failed to write %u-byte firmware to \"%.*s\"", firmware->Length(), (int)firmwarePath.length(), firmwarePath.data());
    } else {
        retro::debug("Flushed %u-byte firmware to \"%.*s\"", firmware->Length(), (int)firmwarePath.length(), firmwarePath.data());
    }
}

// This task keeps running for the lifetime of the task queue.
retro::task::TaskSpec melonds::sram::FlushGbaSramTask(const retro_game_info& gba_save_info) noexcept {
    retro::task::TaskSpec task(
        [info=gba_save_info](retro::task::TaskHandle &task) noexcept {
            ZoneScopedN("melonds::sram::FlushGbaSramTask");

            if (TimeToGbaFlush != nullopt && (*TimeToGbaFlush)-- <= 0) {
                // If it's time to flush the GBA's SRAM...
                retro::debug("GBA SRAM flush timer expired, flushing save data now");
                FlushGbaSram(task, info);
                TimeToGbaFlush = nullopt; // Reset the timer
            }
        },
        nullptr,
        [info=gba_save_info](retro::task::TaskHandle& task) noexcept {
            ZoneScopedN("melonds::sram::FlushGbaSramTask::Cleanup");
            FlushGbaSram(task, info);
            TimeToGbaFlush = nullopt;
        }
    );

    return task;
}

retro::task::TaskSpec melonds::sram::FlushFirmwareTask(string_view path) {
    optional<string> firmwarePath = retro::get_system_path(path);
    if (!firmwarePath) {
        throw std::runtime_error(string("Failed to get system path for firmware named ") + string(path));
    }
    return retro::task::TaskSpec(
        [path=*firmwarePath](retro::task::TaskHandle &) noexcept {
            ZoneScopedN("melonds::sram::FlushFirmwareTask");

            if (TimeToFirmwareFlush != nullopt && (*TimeToFirmwareFlush)-- <= 0) {
                // If it's time to flush the firmware...
                retro::debug("Firmware flush timer expired, flushing data now");
                FlushFirmware(path);
                TimeToFirmwareFlush = nullopt; // Reset the timer
            }
        },
        nullptr,
        [path=*firmwarePath](retro::task::TaskHandle&) noexcept {
            ZoneScopedN("melonds::sram::FlushFirmwareTask::Cleanup");
            FlushFirmware(path);
            TimeToFirmwareFlush = nullopt;
        }
    );
}

// Does not load the NDS SRAM, since retro_get_memory is used for that.
// But it will allocate the SRAM buffer
void melonds::sram::InitNdsSave(const NdsCart &nds_cart) {
    ZoneScopedN("melonds::sram::InitNdsSave");
    using std::runtime_error;
    if (nds_cart.GetHeader().IsHomebrew()) {
        // If this is a homebrew ROM...

        // Homebrew is a special case, as it uses an SD card rather than SRAM.
        // No need to explicitly load or save homebrew SD card images;
        // the CartHomebrew class does that.
        if (config::save::DldiFolderSync()) {
            // If we're syncing the homebrew SD card image to the host filesystem...
            if (!path_mkdir(config::save::DldiFolderPath().c_str())) {
                // Create the directory. If that fails...
                // (note that an existing directory is not an error)
                throw runtime_error("Failed to create virtual SD card directory at " + config::save::DldiFolderPath());
            }
        }
    }
    else {
        // Get the length of the ROM's SRAM, if any
        u32 sram_length = nds_cart.GetSaveMemoryLength();

        if (sram_length > 0) {
            sram::NdsSaveManager = make_unique<SaveManager>(sram_length);
            retro::log(RETRO_LOG_DEBUG, "Allocated %u-byte SRAM buffer for loaded NDS ROM.", sram_length);
        } else {
            retro::log(RETRO_LOG_DEBUG, "Loaded NDS ROM does not use SRAM.");
        }
        // The actual SRAM file is installed later; it's loaded into the core via retro_get_memory_data,
        // and it's applied in the first frame of retro_run.
    }
}

// Loads the GBA SRAM
void melonds::sram::InitGbaSram(GbaCart& gba_cart, const struct retro_game_info& gba_save_info) {
    ZoneScopedN("melonds::sram::InitGbaSram");
    // We load the GBA SRAM file ourselves (rather than letting the frontend do it)
    // because we'll overwrite it later and don't want the frontend to hold open any file handles.
    // Due to libretro limitations, we can't use retro_get_memory_data to load the GBA SRAM
    // without asking the user to move their SRAM into the melonDS DS save folder.
    if (path_contains_compressed_file(gba_save_info.path)) {
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
    rzipstream_t* gba_save_file = rzipstream_open(gba_save_info.path, RETRO_VFS_FILE_ACCESS_READ);
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

    sram::GbaSaveManager = make_unique<SaveManager>(gba_save_file_size);
    gba_cart.SetupSave(gba_save_file_size);
    // LoadSave's subclasses will call Platform::WriteGBASave.
    // The data will be in the buffer soon enough.
    gba_cart.LoadSave(static_cast<const u8*>(gba_save_data), gba_save_file_size);
    retro::debug("Allocated %u-byte GBA SRAM", gba_cart.GetSaveMemoryLength());
    // Actually installing the SRAM will be done later, after NDS::Reset is called
    free(gba_save_data);
    rzipstream_close(gba_save_file);
    retro::task::push(sram::FlushGbaSramTask(gba_save_info));
}

void Platform::WriteNDSSave(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen) {
    // TODO: Implement a Fast SRAM mode where the frontend is given direct access to the SRAM buffer
    ZoneScopedN("Platform::WriteNDSSave");
    if (melonds::sram::NdsSaveManager) {
        melonds::sram::NdsSaveManager->Flush(savedata, savelen, writeoffset, writelen);

        // No need to maintain a flush timer for NDS SRAM,
        // because retro_get_memory lets us delegate autosave to the frontend.
    }
}

void Platform::WriteGBASave(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen) {
    ZoneScopedN("Platform::WriteGBASave");
    if (melonds::sram::GbaSaveManager) {
        melonds::sram::GbaSaveManager->Flush(savedata, savelen, writeoffset, writelen);

        // Start the countdown until we flush the SRAM back to disk.
        // The timer resets every time we write to SRAM,
        // so that a sequence of SRAM writes doesn't result in
        // a sequence of disk writes.
        TimeToGbaFlush = melonds::config::save::FlushDelay();
    }
}

void Platform::WriteFirmware(const SPI_Firmware::Firmware &firmware, u32 writeoffset) {
    ZoneScopedN("Platform::WriteFirmware");

    TimeToFirmwareFlush = melonds::config::save::FlushDelay();
}


