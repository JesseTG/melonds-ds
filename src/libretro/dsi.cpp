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

#include "dsi.hpp"

#include <DSi_NAND.h>

bool melonds::dsi::is_dsiware(const NDSHeader& header) noexcept {
    // TODO: Check the header to see if the right bits are set (look at GBATek for details)
}

void melonds::dsi::install_dsiware(const struct retro_game_info &info) {
    // TODO: Ensure NAND is loaded
    // TODO: If title is already loaded, skip this step (and don't delete it at the end)
    // TODO: Get title metadata from online, or from filesystem
    // TODO: Cache title metadata in the system directory
    // TODO: Import title into NAND
    // TODO: Import Game.public.sav if it exists
    // TODO: Import Game.private.sav if it exists
    // TODO: Import Game.banner.sav if it exists
}

void melonds::dsi::uninstall_dsiware() {
    // TODO: Ensure NAND is loaded
    // TODO: Export Game.public.sav if it exists
    // TODO: Export Game.private.sav if it exists
    // TODO: Export Game.banner.sav if it exists
    // TODO: Delete title from NAND (unless it was already there)
}