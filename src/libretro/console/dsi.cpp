/*
    Copyright 2025 Jesse Talavera

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

#include "console/dsi.hpp"

#include <NDS.h>
#include <DSi.h>
#include <DSi_I2C.h>
#include <retro_assert.h>
#include <compat/strl.h>
#include <file/file_path.h>

#include "environment.hpp"
#include "tracy/client.hpp"

using namespace MelonDsDs;

constexpr size_t DSI_AUTOLOAD_OFFSET = 0x300;

// unknown bit, seems to be required to boot into games (errors otherwise?)
constexpr uint32_t UNKNOWN_BOOT_BIT = (1 << 4);

void MelonDsDs::SetUpDSiWareDirectBoot(melonDS::DSi& dsi, const melonDS::NDSHeader& header) noexcept {
    ZoneScopedN(TracyFunction);

    auto* bptwl = dsi.I2C.GetBPTWL();
    retro_assert(bptwl != nullptr);

    bptwl->SetBootFlag(true);

    // setup "auto-load" feature
    DSiAutoLoad autoLoad {};
    memcpy(autoLoad.ID, "TLNC", sizeof(autoLoad.ID));
    autoLoad.Unknown1 = 0x01;
    autoLoad.Length = 0x18;
    memcpy(autoLoad.NewTitleID, &header.DSiTitleIDLow, sizeof(autoLoad.NewTitleID));
    // Copy header.DSiTitleIDLow and header.DSiTitleIDHigh to autoLoad.NewTitleID

    autoLoad.Flags |= (0x03 << 1) | 0x01 | UNKNOWN_BOOT_BIT;
    autoLoad.Flags |= (1 << 4);
    autoLoad.CRC16 = melonDS::CRC16((uint8_t*)&autoLoad.PrevTitleID, autoLoad.Length, 0xFFFF);
    memcpy(&dsi.MainRAM[DSI_AUTOLOAD_OFFSET], &autoLoad, sizeof(autoLoad));
}