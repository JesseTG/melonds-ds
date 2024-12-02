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
#include "core/core.hpp"
#include "environment.hpp"
#include <fmt/base.h>
#include <retro_assert.h>

using namespace melonDS;

void MelonDsDs::CoreState::MpStarted(retro_netpacket_send_t send, retro_netpacket_poll_receive_t poll_receive) noexcept {
    _mpState.SetSendFn(send);
    _mpState.SetPollFn(poll_receive);
    retro::info("Starting multiplayer on libretro side");
}

void MelonDsDs::CoreState::MpPacketReceived(const void *buf, size_t len) noexcept {
    _mpState.PacketReceived(buf, len);
}

void MelonDsDs::CoreState::MpStopped() noexcept {
    _mpState.SetSendFn(nullptr);
    _mpState.SetPollFn(nullptr);
    retro::info("Stopping multiplayer on libretro side");
}

bool MelonDsDs::CoreState::MpSendPacket(const MelonDsDs::Packet &p) const noexcept {
    if(!_mpState.IsReady()) {
        return false;
    }
    _mpState.SendPacket(p);
    return true;
}

std::optional<MelonDsDs::Packet> MelonDsDs::CoreState::MpNextPacket() noexcept {
    if(!_mpState.IsReady()) {
        return std::nullopt;
    }
    return _mpState.NextPacket();
}

std::optional<MelonDsDs::Packet> MelonDsDs::CoreState::MpNextPacketBlock() noexcept {
    if(!_mpState.IsReady()) {
        return std::nullopt;
    }
    return _mpState.NextPacketBlock();
}

bool MelonDsDs::CoreState::MpActive() const noexcept {
    return _mpState.IsReady();
}

// Not much we can do in Begin and End
void Platform::MP_Begin(void*) {
    retro::info("Starting multiplayer on DS side");
}

void Platform::MP_End(void*) {
    retro::info("Ending multiplayer on DS side");
}
