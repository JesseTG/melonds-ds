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

#include <string>
#include <string_view>
#include <Platform.h>
#include <dynamic/dylib.h>
#include <frontend/qt_sdl/LAN_PCap.h>
#include <frontend/qt_sdl/LAN_Socket.h>
#include <retro_assert.h>
#include <pcap/pcap.h>

#include "../core/core.hpp"
#include "../config/constants.hpp"
#include "../config/config.hpp"
#include "../environment.hpp"
#include "tracy.hpp"
#include "pcap.hpp"

using namespace melonDS;
using std::string;
using std::string_view;
struct Slirp;

namespace LAN_Socket
{
    extern Slirp* Ctx;
}

#ifdef HAVE_NETWORKING_DIRECT_MODE
namespace LAN_PCap
{
    extern Platform::DynamicLibrary* PCapLib;
}
#endif

namespace Config {
    // Needed by melonDS's wi-fi implementation;
    // must be set to some value of Adapter::DeviceName
    string LANDevice;
}

#ifdef HAVE_NETWORKING_DIRECT_MODE
bool MelonDsDs::IsAdapterAcceptable(const LAN_PCap::AdapterData& adapter) noexcept {
    ZoneScopedN(TracyFunction);
    const MacAddress& mac = *reinterpret_cast<const MacAddress*>(adapter.MAC);

    if (mac == BAD_MAC || mac == BROADCAST_MAC)
        return false;

    const pcap_if_t* iface = static_cast<const pcap_if_t*>(adapter.Internal);
    if (iface == nullptr)
        return false;

    if (iface->flags & PCAP_IF_LOOPBACK)
        // If this is a loopback interface...
        return false;

    return true;
}

const LAN_PCap::AdapterData* MelonDsDs::CoreState::SelectNetworkInterface(const LAN_PCap::AdapterData* adapters, int numAdapters) const noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs;


    if (Config.NetworkInterface() != config::values::AUTO) {
        const auto* selected = std::find_if(adapters, adapters + numAdapters, [this](const LAN_PCap::AdapterData& a) {

            string mac = fmt::format("{:02x}", fmt::join(a.MAC, ":"));
            return Config.NetworkInterface() == mac;
        });

        retro_assert(selected != nullptr);
        return selected;
    }

    const auto* best = std::max_element(adapters, adapters + numAdapters, [](const LAN_PCap::AdapterData& a, const LAN_PCap::AdapterData& b) {
        retro_assert(a.Internal != nullptr);
        retro_assert(b.Internal != nullptr);

        const pcap_if_t& a_if = *static_cast<const pcap_if_t*>(a.Internal);
        const pcap_if_t& b_if = *static_cast<const pcap_if_t*>(b.Internal);

        int a_score = 0;
        int b_score = 0;

        // Prefer interfaces that are connected
        if ((a_if.flags & PCAP_IF_CONNECTION_STATUS) == PCAP_IF_CONNECTION_STATUS_CONNECTED)
            a_score += 1000;

        if ((b_if.flags & PCAP_IF_CONNECTION_STATUS) == PCAP_IF_CONNECTION_STATUS_CONNECTED)
            b_score += 1000;

        // Prefer wired interfaces
        if (!(a_if.flags & PCAP_IF_WIRELESS))
            a_score += 100;

        if (!(b_if.flags & PCAP_IF_WIRELESS))
            b_score += 100;

        if (!MelonDsDs::IsAdapterAcceptable(a))
            a_score = INT_MIN;

        if (!MelonDsDs::IsAdapterAcceptable(b))
            b_score = INT_MIN;

        return a_score < b_score;
    });

    retro_assert(best != adapters + numAdapters);

    return best;
}
#endif

bool MelonDsDs::CoreState::LanInit() noexcept {
    ZoneScopedN(TracyFunction);
    retro_assert(_activeNetworkMode == MelonDsDs::NetworkMode::None);
    retro_assert(LAN_Socket::Ctx == nullptr);

    // LAN::PCap may already be initialized if we're using direct mode,
    // as it was necessary to query the available interfaces for the core options
    switch (Config.NetworkMode()) {
#ifdef HAVE_NETWORKING_DIRECT_MODE
        case MelonDsDs::NetworkMode::Direct: {
            const LAN_PCap::AdapterData* adapter = SelectNetworkInterface(LAN_PCap::Adapters, LAN_PCap::NumAdapters);
            Config::LANDevice = adapter->DeviceName;
            if (LAN_PCap::Init(true)) {
                retro::debug(
                    "Initialized direct-mode Wi-fi support with adapter {} ({:02x})\n",
                    adapter->FriendlyName,
                    fmt::join(adapter->MAC, ":")
                );
                _activeNetworkMode = MelonDsDs::NetworkMode::Direct;
                return true;
            } else {
                retro::warn(
                    "Failed to initialize direct-mode Wi-fi support with adapter {} ({:02x}); falling back to indirect mode\n",
                    adapter->FriendlyName,
                    fmt::join(adapter->MAC, ":")
                );
            }
            [[fallthrough]];
        }
#endif
        case MelonDsDs::NetworkMode::Indirect:
            if (LAN_Socket::Init()) {
                retro::debug("Initialized indirect-mode Wi-fi support\n");
                _activeNetworkMode = MelonDsDs::NetworkMode::Indirect;
                return true;
            }

            retro::set_error_message("Failed to initialize indirect-mode Wi-fi support. Wi-fi will not be emulated.");
            [[fallthrough]];
        default:
            _activeNetworkMode = MelonDsDs::NetworkMode::None;
            return false;
    }
}

void MelonDsDs::CoreState::LanDeinit() noexcept {
    ZoneScopedN(TracyFunction);

#ifdef HAVE_NETWORKING_DIRECT_MODE
    LAN_PCap::DeInit();
    retro_assert(LAN_PCap::PCapLib == nullptr);
#endif
    LAN_Socket::DeInit();
    retro_assert(LAN_Socket::Ctx == nullptr);

    _activeNetworkMode = MelonDsDs::NetworkMode::None;
}

int MelonDsDs::CoreState::LanSendPacket(std::span<std::byte> data) noexcept {
    ZoneScopedN(TracyFunction);
    switch (_activeNetworkMode) {
#ifdef HAVE_NETWORKING_DIRECT_MODE
        case MelonDsDs::NetworkMode::Direct:
            return LAN_PCap::SendPacket((u8*)data.data(), data.size());
#endif
        case MelonDsDs::NetworkMode::Indirect:
            return LAN_Socket::SendPacket((u8*)data.data(), data.size());
        default:
            return 0;
    }
}

int MelonDsDs::CoreState::LanRecvPacket(u8 *data) noexcept {
    ZoneScopedN(TracyFunction);
    switch (_activeNetworkMode) {
#ifdef HAVE_NETWORKING_DIRECT_MODE
        case MelonDsDs::NetworkMode::Direct:
            return LAN_PCap::RecvPacket(data);
#endif
        case MelonDsDs::NetworkMode::Indirect:
            return LAN_Socket::RecvPacket(data);
        default:
            return 0;
    }
}

#ifdef HAVE_DYLIB
Platform::DynamicLibrary *Platform::DynamicLibrary_Load(const char *lib) {
    ZoneScopedN(TracyFunction);
    return static_cast<DynamicLibrary *>(dylib_load(lib));
}

void Platform::DynamicLibrary_Unload(Platform::DynamicLibrary *lib) {
    ZoneScopedN(TracyFunction);
    dylib_close(lib);
}

void *Platform::DynamicLibrary_LoadFunction(Platform::DynamicLibrary *lib, const char *name) {
    ZoneScopedN(TracyFunction);
    return reinterpret_cast<void *>(dylib_proc(lib, name));
}
#endif