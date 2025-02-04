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

#pragma once
#include <cstdint>

//!
/// This header concerns DSi-specific functionality

namespace melonDS {
    class NDS;
    class DSi;
    class NDSHeader;
}

namespace MelonDsDs {
    // Taken from BizHawk's melonDS core.
    // TODO: Submit a patch to standalone melonDS
    struct DSiAutoLoad
    {
        uint8_t ID[4]; // "TLNC"
        uint8_t Unknown1; // "usually 01h"
        uint8_t Length; // starting from PrevTitleId
        uint16_t CRC16; // covering Length bytes ("18h=norm")
        uint8_t PrevTitleID[8]; // can be 0 ("anonymous")
        uint8_t NewTitleID[8];
        uint32_t Flags; // bit 0: is valid, bit 1-3: boot type ("01h=Cartridge, 02h=Landing, 03h=DSiware"), other bits unknown/unused
        uint32_t Unused1; // this part is typically still checksummed
        uint8_t Unused2[0xE0]; // this part isn't checksummed, but is 0 filled on erasing autoload data
    };
    static_assert(sizeof(DSiAutoLoad) == 0x100, "DSiAutoLoad must be 256 bytes");

    [[gnu::cold]] void SetUpDSiWareDirectBoot(melonDS::DSi& dsi, const melonDS::NDSHeader& header) noexcept;
}