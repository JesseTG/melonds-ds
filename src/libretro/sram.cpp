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

#include <memory>
#include <retro_assert.h>

#include "memory.hpp"
#include "tracy.hpp"

using std::unique_ptr;
using std::make_unique;

unique_ptr<melonds::SaveManager> melonds::sram::NdsSaveManager;
unique_ptr<melonds::SaveManager> melonds::sram::GbaSaveManager;

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