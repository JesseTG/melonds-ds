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

#ifndef MELONDS_DS_DSI_HPP
#define MELONDS_DS_DSI_HPP

#include <libretro.h>
#include <NDS_Header.h>
#include <NDSCart.h>

#include "libretro.hpp"

namespace melonds::dsi {
    void install_dsiware(const retro_game_info& nds_info, const NdsCart& cart);
    void uninstall_dsiware();
}
#endif //MELONDS_DS_DSI_HPP
