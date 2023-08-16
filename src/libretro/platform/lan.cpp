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

#include <Platform.h>
#include <dynamic/dylib.h>
#include <frontend/qt_sdl/LAN_PCap.h>
#include <frontend/qt_sdl/LAN_Socket.h>
#include <retro_assert.h>

#include "../config.hpp"
#include "../environment.hpp"
#include "tracy.hpp"

static melonds::NetworkMode _activeNetworkMode;
struct Slirp;

namespace LAN_Socket
{
    extern Slirp* Ctx;
}

namespace LAN_PCap
{
    extern Platform::DynamicLibrary* PCapLib;
}

bool Platform::LAN_Init() {
    ZoneScopedN("Platform::LAN_Init");
    using namespace melonds::config::net;
    retro_assert(_activeNetworkMode == melonds::NetworkMode::None);
    retro_assert(LAN_Socket::Ctx == nullptr);
    retro_assert(LAN_PCap::PCapLib == nullptr);
    switch (NetworkMode()) {
        case melonds::NetworkMode::Direct:
            if (LAN_PCap::Init(true)) {
                retro::debug("Initialized direct-mode Wi-fi support");
                _activeNetworkMode = melonds::NetworkMode::Direct;
                return true;
            } else {
                retro::warn("Failed to initialize direct-mode Wi-fi support; falling back to indirect mode");
            }
            [[fallthrough]];
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
    LAN_PCap::DeInit();
    LAN_Socket::DeInit();
    _activeNetworkMode = melonds::NetworkMode::None;
    retro_assert(LAN_Socket::Ctx == nullptr);
    retro_assert(LAN_PCap::PCapLib == nullptr);
}

int Platform::LAN_SendPacket(u8 *data, int len) {
    ZoneScopedN("Platform::LAN_SendPacket");
    switch (_activeNetworkMode) {
        case melonds::NetworkMode::Direct:
            return LAN_PCap::SendPacket(data, len);
        case melonds::NetworkMode::Indirect:
            return LAN_Socket::SendPacket(data, len);
        default:
            return 0;
    }
}

int Platform::LAN_RecvPacket(u8 *data) {
    ZoneScopedN("Platform::LAN_RecvPacket");
    switch (_activeNetworkMode) {
        case melonds::NetworkMode::Direct:
            return LAN_PCap::RecvPacket(data);
        case melonds::NetworkMode::Indirect:
            return LAN_Socket::RecvPacket(data);
        default:
            return 0;
    }
}

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
