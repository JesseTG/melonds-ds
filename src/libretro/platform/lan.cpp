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

#include "tracy.hpp"
#include "core/core.hpp"

using namespace melonDS;

int MelonDsDs::CoreState::LanSendPacket(std::span<std::byte> data) noexcept {
    ZoneScopedN(TracyFunction);

    return _netState.SendPacket(data);
}

int MelonDsDs::CoreState::LanRecvPacket(u8 *data) noexcept {
    ZoneScopedN(TracyFunction);

    return _netState.RecvPacket(data);
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