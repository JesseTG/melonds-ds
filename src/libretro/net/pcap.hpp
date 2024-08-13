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

#ifndef MELONDS_DS_PCAP_HPP
#define MELONDS_DS_PCAP_HPP

#include <array>

#ifdef HAVE_NETWORKING_DIRECT_MODE
#include <pcap/pcap.h>
#include <Net_PCap.h>
#define PCAP_IF_WIRELESS                                0x00000008      /* interface is wireless (*NOT* necessarily Wi-Fi!) */
#define PCAP_IF_CONNECTION_STATUS                       0x00000030      /* connection status: */
#define PCAP_IF_CONNECTION_STATUS_UNKNOWN               0x00000000      /* unknown */
#define PCAP_IF_CONNECTION_STATUS_CONNECTED             0x00000010      /* connected */
#define PCAP_IF_CONNECTION_STATUS_DISCONNECTED          0x00000020      /* disconnected */
#define PCAP_IF_CONNECTION_STATUS_NOT_APPLICABLE        0x00000030      /* not applicable */

namespace melonDS
{
    struct AdapterData;
}

namespace MelonDsDs {
    constexpr std::array<uint8_t, 6> BAD_MAC = {0, 0, 0, 0, 0, 0};
    constexpr std::array<uint8_t, 6> BROADCAST_MAC = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    bool IsAdapterAcceptable(const melonDS::AdapterData& adapter) noexcept;
}
#endif
#endif //MELONDS_DS_PCAP_HPP
