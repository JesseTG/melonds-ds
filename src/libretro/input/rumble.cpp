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

#include <algorithm>
#include <cmath>

#include <libretro.h>

#include "config/config.hpp"
#include "environment.hpp"
#include "tracy/client.hpp"

using MelonDsDs::RumbleEnvelope;
using MelonDsDs::RumbleState;

void RumbleEnvelope::Push(uint32_t edges) noexcept {
    std::copy_backward(_samples.begin(), _samples.end() - 1, _samples.end());
    _samples[0] = std::min(static_cast<float>(edges) / RUMBLE_SATURATION_EDGES_PER_FRAME, 1.0f);
}

float RumbleEnvelope::Level() const noexcept {
    // Older frames count for less than newer ones, right down to nothing at the
    // end of the window. A flat average would hold a one-frame buzz at a
    // constant level for the whole window and then cut it off dead; weighting
    // it this way turns that same buzz into something that fades.
    float weighted = 0.0f;
    float total = 0.0f;
    for (size_t i = 0; i < _samples.size(); ++i) {
        float weight = static_cast<float>(_samples.size() - i);
        weighted += _samples[i] * weight;
        total += weight;
    }

    // Smoothing the way up too would delay every buzz by a few frames.
    // Rumble that lags behind the action is far more noticeable
    // than rumble that lingers, so the newest frame bypasses the filter.
    return std::max(_samples[0], weighted / total);
}

void RumbleEnvelope::Clear() noexcept {
    _samples.fill(0.0f);
}

RumbleState::~RumbleState() noexcept {
    if (_armed) {
        retro::set_rumble_state(0, 0);
    }
}

RumbleState::RumbleState(RumbleState&& other) noexcept :
    _envelope(other._envelope),
    _edges(other._edges),
    _lastEdges(other._lastEdges),
    _level(other._level),
    _lastStrength(other._lastStrength),
    _intensity(other._intensity),
    _motors(other._motors),
    _armed(other._armed) {
    // Only one Rumble Pak can be in Slot-2 at a time,
    // so only one RumbleState may drive the motors.
    other._armed = false;
}

RumbleState& RumbleState::operator=(RumbleState&& other) noexcept {
    if (this != &other) {
        if (_armed && _lastStrength != 0) {
            // Whatever we were rumbling for is over; don't leave the motors on.
            retro::set_rumble_state(0, 0);
        }

        _envelope = other._envelope;
        _edges = other._edges;
        _lastEdges = other._lastEdges;
        _level = other._level;
        _lastStrength = other._lastStrength;
        _intensity = other._intensity;
        _motors = other._motors;
        _armed = other._armed;
        other._armed = false;
    }

    return *this;
}

void RumbleState::SetConfig(const CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    _intensity = config.RumbleIntensity();
    _motors = config.RumbleMotorType();
}

void RumbleState::Update() noexcept {
    ZoneScopedN(TracyFunction);

    _lastEdges = _edges;
    _envelope.Push(_edges);
    _edges = 0;

    long scaled = std::lround(_envelope.Level() * static_cast<float>(_intensity));
    _level = static_cast<uint16_t>(std::clamp<long>(scaled, 0, UINT16_MAX));

    TracyPlot("Rumble Pak Edges", static_cast<int64_t>(_lastEdges));
    TracyPlot("Rumble Strength", static_cast<int64_t>(_level));

    // Don't pester the frontend's haptic driver with a stream of zeroes
    // when the game isn't asking for any rumble;
    // that's a syscall per frame per motor for nothing.
    if (_level != 0 || _lastStrength != 0) {
        Emit(_level);
        _lastStrength = _level;
    }
}

void RumbleState::Stop() noexcept {
    ZoneScopedN(TracyFunction);
    _envelope.Clear();
    _edges = 0;
    _lastEdges = 0;
    _level = 0;

    if (_lastStrength != 0) {
        Emit(0);
        _lastStrength = 0;
    }
}

void RumbleState::Emit(uint16_t strength) const noexcept {
    retro::set_rumble_state(
        0, RETRO_RUMBLE_STRONG, (_motors & RumbleMotorType::Strong) ? strength : 0
    );
    retro::set_rumble_state(
        0, RETRO_RUMBLE_WEAK, (_motors & RumbleMotorType::Weak) ? strength : 0
    );
}
