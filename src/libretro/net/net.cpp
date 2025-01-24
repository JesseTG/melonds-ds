/*
    Copyright 2024 Jesse Talavera

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

#include "net.hpp"

#include <string_view>

#include <Net_Slirp.h>
#include <retro_assert.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "environment.hpp"
#include "config/config.hpp"
#include "pcap.hpp"
#include "tracy.hpp"

using std::vector;
using namespace melonDS;

#ifdef HAVE_NETWORKING_DIRECT_MODE
bool MelonDsDs::IsAdapterAcceptable(const AdapterData& adapter) noexcept
{
    ZoneScopedN(TracyFunction);

    if (const MacAddress& mac = *reinterpret_cast<const MacAddress*>(adapter.MAC); mac == BAD_MAC || mac ==
        BROADCAST_MAC)
        return false;

    if (adapter.Flags & PCAP_IF_LOOPBACK)
        // If this is a loopback interface...
        return false;

    return true;
}

const AdapterData* SelectNetworkInterface(std::string_view iface, std::span<const AdapterData> adapters) noexcept
{
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs;


    if (iface != config::values::AUTO)
    {
        // If we're explicitly selecting a particular interface...
        const auto* selected = std::find_if(adapters.begin(), adapters.end(), [iface](const AdapterData& a)
        {
            string mac = fmt::format("{:02x}", fmt::join(a.MAC, ":"));
            return iface == mac;
        });

        return selected != adapters.end() ? selected : nullptr;
    }

    const auto* best = std::max_element(adapters.begin(), adapters.end(), [](const AdapterData& a, const AdapterData& b)
    {
        u32 a_flags = a.Flags;
        u32 b_flags = b.Flags;
        int a_score = 0;
        int b_score = 0;

        // Prefer interfaces that are connected
        if ((a_flags & PCAP_IF_CONNECTION_STATUS) == PCAP_IF_CONNECTION_STATUS_CONNECTED)
            a_score += 1000;

        if ((b_flags & PCAP_IF_CONNECTION_STATUS) == PCAP_IF_CONNECTION_STATUS_CONNECTED)
            b_score += 1000;

        // Prefer wired interfaces
        if (!(a_flags & PCAP_IF_WIRELESS))
            a_score += 100;

        if (!(b_flags & PCAP_IF_WIRELESS))
            b_score += 100;

        if (!MelonDsDs::IsAdapterAcceptable(a))
            a_score = INT_MIN;

        if (!MelonDsDs::IsAdapterAcceptable(b))
            b_score = INT_MIN;

        return a_score < b_score;
    });

    retro_assert(best != adapters.end());

    return best;
}
#endif

MelonDsDs::NetState::NetState()
#ifdef HAVE_NETWORKING_DIRECT_MODE
    : _pcap(melonDS::LibPCap::New())
#endif
{
    _net.RegisterInstance(0);
    // TODO: Handle registration properly (not yet sure what that'll entail)
}

MelonDsDs::NetState::~NetState() noexcept
{
    _net.UnregisterInstance(0);
}

int MelonDsDs::NetState::SendPacket(std::span<std::byte> data) noexcept
{
    return _net.SendPacket(reinterpret_cast<u8*>(data.data()), data.size(), 0);
}

int MelonDsDs::NetState::RecvPacket(u8* data) noexcept
{
    return _net.RecvPacket(data, 0);
}

vector<melonDS::AdapterData> MelonDsDs::NetState::GetAdapters() const noexcept
{
    ZoneScopedN(TracyFunction);

#ifdef HAVE_NETWORKING_DIRECT_MODE
    if (_pcap)
    {
        return _pcap->GetAdapters();
    }
#endif

    return {};
}

bool operator==(const melonDS::AdapterData& lhs, const melonDS::AdapterData& rhs)
{
    return
        lhs.Flags == rhs.Flags &&
        memcmp(lhs.MAC, rhs.MAC, sizeof(lhs.MAC)) == 0 &&
        memcmp(lhs.IP_v4, rhs.IP_v4, sizeof(lhs.IP_v4)) == 0 &&
        strncmp(lhs.Description, rhs.Description, sizeof(lhs.Description)) == 0 &&
        strncmp(lhs.FriendlyName, rhs.FriendlyName, sizeof(lhs.FriendlyName)) == 0 &&
        strncmp(lhs.DeviceName, rhs.DeviceName, sizeof(lhs.DeviceName)) == 0
    ;
}

void MelonDsDs::NetState::Apply(const CoreConfig& config) noexcept
{
    ZoneScopedN(TracyFunction);

    NetworkMode lastMode = GetNetworkMode();

    switch (config.NetworkMode())
    {
#ifdef HAVE_NETWORKING_DIRECT_MODE
    case NetworkMode::Direct:
        if (!_pcap)
        { // If a previous attempt to load libpcap failed...
            _pcap = melonDS::LibPCap::New(); // ...then try again.
            // (This can happen if the player installed it with RetroArch running in the background)
        }

        if (_pcap)
        {
            auto adapters = _pcap->GetAdapters();

            if (const AdapterData* adapter = SelectNetworkInterface(config.NetworkInterface(), adapters); adapter)
            {
                if (lastMode == NetworkMode::Direct && _adapter && *adapter == *_adapter)
                { // If we were already using direct-mode, and with the same selected adapter...
                    retro::debug(
                        "Already using direct-mode Wi-fi support with adapter {} ({:02x}); no need to reset\n",
                        adapter->FriendlyName,
                        fmt::join(adapter->MAC, ":")
                    );

                    return;
                }

                auto driver = _pcap->Open(*adapter, [this](const u8* data, int len)
                {
                    _net.RXEnqueue(data, len);
                });

                if (driver)
                {
                    retro::debug(
                        "Initialized direct-mode Wi-fi support with adapter {} ({:02x})\n",
                        adapter->FriendlyName,
                        fmt::join(adapter->MAC, ":")
                    );
                    _net.SetDriver(std::move(driver));
                    _adapter = *adapter;
                    return;
                }
                else
                {
                    retro::warn(
                        "Failed to initialize direct-mode Wi-fi support with adapter {} ({:02x}); falling back to indirect mode\n",
                        adapter->FriendlyName,
                        fmt::join(adapter->MAC, ":")
                    );
                }
            }
        }
        else
        {
            retro::set_warn_message("Failed to load libpcap. Falling back to indirect mode.");
        }

        [[fallthrough]];
#endif

    case NetworkMode::Indirect:
        if (lastMode != NetworkMode::Indirect)
        {
            // If we're not already using indirect mode...
            _net.SetDriver(std::make_unique<Net_Slirp>([this](const u8* data, int len)
            {
                _net.RXEnqueue(data, len);
            }));

#ifdef HAVE_NETWORKING_DIRECT_MODE
            _adapter = std::nullopt;
#endif

            retro::debug("Initialized indirect-mode Wi-fi support\n");
        }
        else
        {
            retro::debug("Already using indirect mode, no need to reset network driver\n");
        }

        break;

    case NetworkMode::None:
        _net.SetDriver(nullptr);
#ifdef HAVE_NETWORKING_DIRECT_MODE
        _adapter = std::nullopt;
#endif
    }
}

[[nodiscard]] MelonDsDs::NetworkMode MelonDsDs::NetState::GetNetworkMode() const noexcept
{
#ifdef HAVE_NETWORKING_DIRECT_MODE
    if (dynamic_cast<const Net_PCap*>(_net.GetDriver().get()))
    {
        return NetworkMode::Direct;
    }
#endif

    if (dynamic_cast<const Net_Slirp*>(_net.GetDriver().get()))
    {
        return NetworkMode::Indirect;
    }

    return NetworkMode::None;
}
