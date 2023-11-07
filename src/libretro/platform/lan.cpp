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

#include "../config/constants.hpp"
#include "../config.hpp"
#include "../environment.hpp"
#include "tracy.hpp"
#include "pcap.hpp"

using std::string;
using std::string_view;
static melonds::NetworkMode _activeNetworkMode;
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
bool melonds::IsAdapterAcceptable(const LAN_PCap::AdapterData& adapter) noexcept {
    ZoneScopedN("Platform::LAN_Init::LAN_PCap::IsAdapterAcceptable");
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

static const LAN_PCap::AdapterData* SelectNetworkInterface(const LAN_PCap::AdapterData* adapters, int numAdapters) noexcept {
    ZoneScopedN("Platform::LAN_Init::LAN_PCap::SelectNetworkInterface");
    using namespace melonds;


    if (config::net::NetworkInterface() != config::values::AUTO) {
        const auto* selected = std::find_if(adapters, adapters + numAdapters, [](const LAN_PCap::AdapterData& a) {

            string mac = fmt::format("{:02x}", fmt::join(a.MAC, ":"));
            return config::net::NetworkInterface() == mac;
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

        if (!melonds::IsAdapterAcceptable(a))
            a_score = INT_MIN;

        if (!melonds::IsAdapterAcceptable(b))
            b_score = INT_MIN;

        return a_score < b_score;
    });

    retro_assert(best != adapters + numAdapters);

    return best;
}
#endif

bool Platform::LAN_Init() {
    ZoneScopedN("Platform::LAN_Init");
    using namespace melonds::config::net;
    retro_assert(_activeNetworkMode == melonds::NetworkMode::None);
    retro_assert(LAN_Socket::Ctx == nullptr);

    // LAN::PCap may already be initialized if we're using direct mode,
    // as it was necessary to query the available interfaces for the core options
    switch (NetworkMode()) {
#ifdef HAVE_NETWORKING_DIRECT_MODE
        case melonds::NetworkMode::Direct: {
            const LAN_PCap::AdapterData* adapter = SelectNetworkInterface(LAN_PCap::Adapters, LAN_PCap::NumAdapters);
            Config::LANDevice = adapter->DeviceName;
            if (LAN_PCap::Init(true)) {
                retro::debug(
                    "Initialized direct-mode Wi-fi support with adapter {} ({:02x})\n",
                    adapter->FriendlyName,
                    fmt::join(adapter->MAC, ":")
                );
                _activeNetworkMode = melonds::NetworkMode::Direct;
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
        case melonds::NetworkMode::Indirect:
            if (LAN_Socket::Init()) {
                retro::debug("Initialized indirect-mode Wi-fi support\n");
                _activeNetworkMode = melonds::NetworkMode::Indirect;
                return true;
            }

            retro::set_error_message("Failed to initialize indirect-mode Wi-fi support. Wi-fi will not be emulated.");
            [[fallthrough]];
        default:
            _activeNetworkMode = melonds::NetworkMode::None;
            return false;
    }
}

void Platform::LAN_DeInit() {
    ZoneScopedN("Platform::LAN_DeInit");
#ifdef HAVE_NETWORKING_DIRECT_MODE
    LAN_PCap::DeInit();
    retro_assert(LAN_PCap::PCapLib == nullptr);
#endif
    LAN_Socket::DeInit();
    retro_assert(LAN_Socket::Ctx == nullptr);

    _activeNetworkMode = melonds::NetworkMode::None;
}

int Platform::LAN_SendPacket(u8 *data, int len) {
    ZoneScopedN("Platform::LAN_SendPacket");
    switch (_activeNetworkMode) {
#ifdef HAVE_NETWORKING_DIRECT_MODE
        case melonds::NetworkMode::Direct:
            return LAN_PCap::SendPacket(data, len);
#endif
        case melonds::NetworkMode::Indirect:
            return LAN_Socket::SendPacket(data, len);
        default:
            return 0;
    }
}

int Platform::LAN_RecvPacket(u8 *data) {
    ZoneScopedN("Platform::LAN_RecvPacket");
    switch (_activeNetworkMode) {
#ifdef HAVE_NETWORKING_DIRECT_MODE
        case melonds::NetworkMode::Direct:
            return LAN_PCap::RecvPacket(data);
#endif
        case melonds::NetworkMode::Indirect:
            return LAN_Socket::RecvPacket(data);
        default:
            return 0;
    }
}

#ifdef HAVE_DYLIB
Platform::DynamicLibrary *Platform::DynamicLibrary_Load(const char *lib) {
    ZoneScopedN("Platform::DynamicLibrary_Load");
    return static_cast<DynamicLibrary *>(dylib_load(lib));
}

void Platform::DynamicLibrary_Unload(Platform::DynamicLibrary *lib) {
    ZoneScopedN("Platform::DynamicLibrary_Unload");
    dylib_close(lib);
}

void *Platform::DynamicLibrary_LoadFunction(Platform::DynamicLibrary *lib, const char *name) {
    ZoneScopedN("Platform::DynamicLibrary_LoadFunction");
    return reinterpret_cast<void *>(dylib_proc(lib, name));
}
#endif