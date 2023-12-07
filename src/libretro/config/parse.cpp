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

#include "parse.hpp"

#include <optional>

#include <net/net_compat.h>

std::optional<melonDS::IpAddress> MelonDsDs::ParseIpAddress(std::string_view value) noexcept {
    ZoneScopedN(TracyFunction);
    if (value.empty())
        return std::nullopt;

    in_addr address;
    if (inet_pton(AF_INET, value.data(), &address) == 1) {
        // Both in_addr and ip represent an IPv4 address,
        // but they may have different alignment requirements.
        // Better safe than sorry.
        melonDS::IpAddress ip {};
        memcpy(&ip, &address, sizeof(address));
        return ip;
    }

    return std::nullopt;
}