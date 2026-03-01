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

#include "rumble.hpp"

#include <optional>

#include "constants.hpp"
#include "environment.hpp"
#include "retro/task_queue.hpp"
#include "tracy/client.hpp"

using MelonDsDs::RumbleState;
using namespace std::chrono_literals;

// We add a bit of decay so the rumble doesn't feel too instant.
// TODO: Make customizable?
constexpr double RUMBLE_DECAY = 0.5;

void RumbleState::RumbleStart(std::chrono::milliseconds len) noexcept {
    _rumbleTimeout += len;
    retro::set_rumble_state(0, 0xFFFF);
}

void RumbleState::RumbleStop() noexcept {
    _rumbleTimeout = 0us;
    retro::set_rumble_state(0, 0);
}

// We need a rumble task because the emulated Rumble Pak is edge-triggered
// (i.e. turned on and off rapidly), and the frontend's rumble API is level-based.
retro::task::TaskSpec RumbleState::RumbleTask() noexcept {
    ZoneScopedN(TracyFunction);

    return retro::task::TaskSpec(
        [this](retro::task::TaskHandle&) noexcept {
            ZoneScopedN(TracyFunction);
            std::optional<std::chrono::microseconds> last_frame_time = retro::last_frame_time();
            if (!last_frame_time) {
                last_frame_time = US_PER_FRAME;
            }

            bool was_rumbling = _rumbleTimeout > 0us;
            _rumbleTimeout -= std::chrono::microseconds(static_cast<int>(last_frame_time->count() * RUMBLE_DECAY));
            if (_rumbleTimeout <= 0us) {
                _rumbleTimeout = 0us;
                if (was_rumbling) {
                    retro::set_rumble_state(0, 0);
                }
            }
        },
        nullptr,
        [](retro::task::TaskHandle&) noexcept {
            retro::set_rumble_state(0, 0);
        },
        retro::task::ASAP,
        "RumbleTask"
    );
}
