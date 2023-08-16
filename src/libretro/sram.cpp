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
#include <retro_assert.h>

#include "tracy.hpp"

using std::unique_ptr;
using std::make_unique;

unique_ptr<melonds::sram::SaveManager> melonds::sram::NdsSaveManager;
unique_ptr<melonds::sram::SaveManager> melonds::sram::GbaSaveManager;

void melonds::sram::init() {
    ZoneScopedN("melonds::sram::init");
    retro_assert(NdsSaveManager == nullptr);
    retro_assert(GbaSaveManager == nullptr);
    NdsSaveManager = make_unique<SaveManager>();
    GbaSaveManager = make_unique<SaveManager>();
}

void melonds::sram::deinit() noexcept {
    ZoneScopedN("melonds::sram::deinit");
    NdsSaveManager = nullptr;
    GbaSaveManager = nullptr;
}

melonds::sram::SaveManager::SaveManager() :
        _sram(nullptr),
        _sram_length(0),
        _buffer_length(0) {
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

void melonds::sram::SaveManager::SetSaveSize(u32 savelen) {
    ZoneScopedN("melonds::sram::SaveManager::SetSaveSize");
    if (_sram_length != savelen) {
        delete[] _sram;

        _sram_length = savelen;
        _sram = new u8[_sram_length];
    }
}