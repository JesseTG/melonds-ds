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

#include "fat.hpp"

#include <cstring>
#include <unistd.h>

#include <Platform.h>

#include "config.hpp"
#include "environment.hpp"

static std::unordered_map<FILE*, int> flushTimers;

u32 Platform::WriteFATSectors(const u8* data, u32 length, u32 count, FILE* file)
{
    u32 blocksWritten = fwrite(data, length, count, file);

    flushTimers[file] =  melonds::config::save::FlushDelay();

    return blocksWritten;
}

void melonds::fat::deinit() {
    flushTimers.clear();
}

retro::task::TaskSpec melonds::fat::FlushTask() noexcept {
    retro::task::TaskSpec task([](retro::task::TaskHandle &task) {
        using namespace melonds::fat;
        if (task.IsCancelled()) {
            // If it's time to stop...
            task.Finish();
            return;
        }

        std::vector<FILE*> filesToRemove;

        for (auto& [file, timeUntilFlush] : flushTimers) {
            timeUntilFlush--;
            if (timeUntilFlush == 0) {
                // If the timer has reached 0, flush the file and remove it from the map
                int fd = fileno(file);
                if (fd >= 0) {
                    // If this file is still open...

#ifdef _WIN32
                    int syncResult = _commit(fd);
#else
                    int syncResult = fsync(fd);
#endif

                    if (syncResult == -1) {
                        int error = errno;
                        retro::error("Failed to flush emulated FAT filesystem to host disk: %s (%x)\n", strerror(error), error);
                    }
                    else {
                        retro::debug("Flushed emulated FAT filesystem to host disk\n");
                    }
                }
                // If the file is not open, then it was closed before the flush timer reached 0.
                // In that case it will have been flushed to disk anyway, so we don't need to do anything.

                filesToRemove.push_back(file);
            }
        }

        for (auto& file : filesToRemove) {
            flushTimers.erase(file);
        }
    });

    return task;
}