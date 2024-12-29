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

#ifndef MELONDS_DS_NETWORK_HPP
#define MELONDS_DS_NETWORK_HPP

#include <libretro.h>

#include "../constants.hpp"

namespace MelonDsDs::config::definitions {
    constexpr retro_core_option_v2_definition NetworkMode {
        config::network::NETWORK_MODE,
        "Networking Mode",
        nullptr,
        "Configures how melonDS DS emulates Nintendo WFC. If unsure, use Indirect mode.\n"
        "\n"
        "Indirect: Use libslirp to emulate the DS's network stack. Simple and needs no setup.\n"
#ifdef HAVE_NETWORKING_DIRECT_MODE
        "Direct: Routes emulated Wi-Fi packets to the host's network interface. "
        "Faster and more reliable, but requires an ethernet connection and "
#ifdef _WIN32
        "that WinPcap or Npcap is installed. "
#else
        "that libpcap is installed. "
#endif
        "If unavailable, falls back to Indirect mode.\n"
#endif
        "\n"
        "Changes take effect at next restart. "
        "Not related to local multiplayer.",
        nullptr,
        config::network::CATEGORY,
        {
            {MelonDsDs::config::values::DISABLED, nullptr},
            {MelonDsDs::config::values::INDIRECT, "Indirect"},
#ifdef HAVE_NETWORKING_DIRECT_MODE
            {MelonDsDs::config::values::DIRECT, "Direct"},
#endif
            {nullptr, nullptr},
        },
        MelonDsDs::config::values::INDIRECT
    };

#ifdef HAVE_NETWORKING_DIRECT_MODE
    constexpr retro_core_option_v2_definition NetworkInterface {
        config::network::DIRECT_NETWORK_INTERFACE,
        "Network Interface (Direct Mode)",
        "Interface (Direct Mode)",
        "Select a network interface to use with Direct Mode. "
        "If unsure, set to Automatic. "
        "Changes take effect at next core restart.",
        nullptr,
        config::network::CATEGORY,
        {
            {MelonDsDs::config::values::AUTO, "Automatic"},
        },
        MelonDsDs::config::values::AUTO
    };
#endif
    constexpr retro_core_option_v2_definition LanMacAddressMode {
        config::network::MAC_ADDRESS_MODE,
        "Network MAC Address Mode",
        "MAC Address Mode",
        "Configures how the emulated console's MAC address is set. "
        "Changing this option might make local multiplayer impossible or block access to save files "
        "in games that use the MAC address to prevent tampering of save files (i.e. Pokémon).\n"
        "No relation to the direct mode interface. Changes take effect at next restart.\n"
        "See https://github.com/jessetg/melonds-ds/blob/main/LanMultiplayer.md for more information.",
        nullptr,
        config::network::CATEGORY,
        {
            {MelonDsDs::config::values::FIRMWARE, "Set from firmware"},
            {MelonDsDs::config::values::FROM_USERNAME, "Derive from libretro username"},
            {nullptr, nullptr}
        },
        MelonDsDs::config::values::FIRMWARE
    };

    constexpr std::initializer_list<retro_core_option_v2_definition> NetworkOptionDefinitions {
#ifdef HAVE_NETWORKING
        NetworkMode,
#   ifdef HAVE_NETWORKING_DIRECT_MODE
        NetworkInterface,
#   endif
#endif
        LanMacAddressMode,
    };
}

#endif //MELONDS_DS_NETWORK_HPP
