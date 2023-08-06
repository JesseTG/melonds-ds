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
#include "tracy.hpp"

using std::nullopt;
using std::optional;
using retro::task::TaskSpec;

namespace melonds::gba {
    std::unique_ptr<melonds::SaveManager> GbaSaveManager = std::make_unique<melonds::SaveManager>();
    static optional<int> TimeToGbaFlush = nullopt;
}

void Platform::WriteGBASave(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen) {
    ZoneScopedN("Platform::WriteGBASave");
    if (melonds::gba::GbaSaveManager) {
        melonds::gba::GbaSaveManager->Flush(savedata, savelen, writeoffset, writelen);

        // Start the countdown until we flush the SRAM back to disk.
        // The timer resets every time we write to SRAM,
        // so that a sequence of SRAM writes doesn't result in
        // a sequence of disk writes.
        melonds::gba::TimeToGbaFlush = melonds::config::save::FlushDelay();
    }
}

void melonds::gba::FlushSram(const retro_game_info& gba_save_info) noexcept {
    ZoneScopedN("melonds::gba::FlushSram");
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
    } else {
        retro::debug("Flushed %u-byte GBA SRAM to \"%s\"", gba_sram_length, save_data_path);
    }
}

// This task keeps running for the lifetime of the task queue.
TaskSpec melonds::gba::FlushTask() noexcept {
    TaskSpec task([](retro::task::TaskHandle &task) {
        ZoneScopedN("melonds::gba::FlushTask");
        using namespace melonds::gba;
        if (task.IsCancelled()) {
            // If it's time to stop...
            task.Finish();
            return;
        }

        if (TimeToGbaFlush == nullopt) {
            // If we don't have a GBA SRAM flush coming up...
            return; // ...then there's nothing to do.
        }

        if ((*TimeToGbaFlush)-- <= 0) {
            // If it's time to flush the GBA's SRAM...
            if (const optional<retro_game_info>& gba_save_info = retro::content::get_loaded_gba_save_info()) {
                // If we actually have GBA save data loaded...
                retro::debug("GBA SRAM flush timer expired, flushing save data now");
                FlushSram(*gba_save_info);
            }
            TimeToGbaFlush = nullopt; // Reset the timer
        }

    });

    return task;
}

void Platform::EnterGBAMode() {
    retro::set_error_message("GBA mode is not supported. Use a GBA core instead.");
    retro::shutdown();
}