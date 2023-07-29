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

#include "gba.hpp"

#include <Platform.h>
#include <libretro.h>
#include <streams/file_stream.h>

#include "config.hpp"
#include "content.hpp"
#include "environment.hpp"
#include "memory.hpp"

using std::optional;

std::unique_ptr<melonds::SaveManager> melonds::gba::GbaSaveManager = std::make_unique<melonds::SaveManager>();
std::optional<int> melonds::gba::TimeToGbaFlush = std::nullopt;

void Platform::WriteGBASave(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen) {
    if (melonds::gba::GbaSaveManager) {
        melonds::gba::GbaSaveManager->Flush(savedata, savelen, writeoffset, writelen);

        // Start the countdown until we flush the SRAM back to disk.
        // The timer resets every time we write to SRAM,
        // so that a sequence of SRAM writes doesn't result in
        // a sequence of disk writes.
        melonds::gba::TimeToGbaFlush = melonds::config::save::FlushDelay();
        // TODO: Take a timestamp of when this function is called,
        // and have the flush task query it;
        // TODO: Schedule a task to flush the SRAM to disk
    }
}

void melonds::gba::FlushSaveData() noexcept {
    if (TimeToGbaFlush != std::nullopt) {
        if (*TimeToGbaFlush > 0) { // std::optional::operator> checks the optional's validity for us
            // If we have a GBA SRAM flush coming up...
            *TimeToGbaFlush -= 1;
        }

        if (*TimeToGbaFlush <= 0) {
            // If it's time to flush the GBA's SRAM...
            const std::optional<retro_game_info>& gba_save_info = retro::content::get_loaded_gba_save_info();
            if (gba_save_info) {
                // If we actually have GBA save data loaded...
                FlushSram(*gba_save_info);
            }
            TimeToGbaFlush = std::nullopt; // Reset the timer
        }
    }
}

void melonds::gba::FlushSram(const retro_game_info& gba_save_info) noexcept {

    const char* save_data_path = gba_save_info.path;
    if (save_data_path == nullptr || GbaSaveManager == nullptr) {
        // No save data path was provided, or the GBA save manager isn't initialized
        return; // TODO: Report this error
    }
    const u8* gba_sram = GbaSaveManager->Sram();
    u32 gba_sram_length = GbaSaveManager->SramLength();

    if (gba_sram == nullptr || gba_sram_length == 0) {
        return; // TODO: Report this error
    }

    if (!filestream_write_file(save_data_path, gba_sram, gba_sram_length)) {
        retro::error("Failed to write %u-byte GBA SRAM to \"%s\"", gba_sram_length, save_data_path);
        // TODO: Report this to the user
    }
    else {
        retro::debug("Flushed %u-byte GBA SRAM to \"%s\"", gba_sram_length, save_data_path);
    }
}