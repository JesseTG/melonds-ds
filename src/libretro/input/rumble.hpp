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

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "config/types.hpp"

namespace MelonDsDs {
    class CoreConfig;

    /// The number of Rumble Pak register toggles within a single emulated frame
    /// that corresponds to the strongest rumble the frontend can produce.
    ///
    /// The Rumble Pak's actuator has no on/off setting;
    /// it lurches once each time the game flips the register,
    /// so the rate of those flips is what the player feels as intensity.
    /// At the DS's frame rate this works out to about 240 toggles per second.
    ///
    /// This is an empirical value, not one derived from the hardware.
    /// periph_slot2.nds emits a 6-toggle burst per buzz;
    /// retune this against a commercial Rumble Pak game
    /// (Metroid Prime Pinball is the one it was bundled with)
    /// by watching the "Rumble Pak Edges" plot in a Tracy-enabled build.
    constexpr float RUMBLE_SATURATION_EDGES_PER_FRAME = 4.0f;

    /// How many frames of toggle counts the rumble envelope looks back over.
    ///
    /// This is how long a buzz takes to fade out (about 100ms) *and* how finely
    /// the strength can be graded: a game only produces a handful of toggles
    /// per frame, so blending several frames together is what gives the level
    /// more than a few distinct values.
    constexpr std::size_t RUMBLE_WINDOW_FRAMES = 6;

    /// A low-pass filter over the Rumble Pak's per-frame toggle counts.
    ///
    /// The Rumble Pak is driven far faster than a frontend's rumble motors
    /// can be told about, so the toggles are buffered for a frame at a time
    /// and smoothed into a single level, much like a very coarse audio signal.
    class RumbleEnvelope {
    public:
        /// Records one emulated frame's worth of register toggles.
        void Push(uint32_t edges) noexcept;

        /// The rumble strength implied by the recent toggle history, from 0 to 1.
        [[nodiscard]] float Level() const noexcept;

        /// Forgets all buffered toggle counts.
        void Clear() noexcept;

    private:
        /// Normalized toggle rates, newest first.
        std::array<float, RUMBLE_WINDOW_FRAMES> _samples {};
    };

    /// Translates the emulated Rumble Pak's activity into frontend rumble.
    ///
    /// Owns the frontend's motors for as long as a Rumble Pak is in Slot-2;
    /// they're switched off when this object is destroyed or moved from.
    class RumbleState {
    public:
        RumbleState() noexcept = default;
        ~RumbleState() noexcept;
        RumbleState(const RumbleState&) = delete;
        RumbleState& operator=(const RumbleState&) = delete;
        RumbleState(RumbleState&&) noexcept;
        RumbleState& operator=(RumbleState&&) noexcept;

        void SetConfig(const CoreConfig& config) noexcept;

        /// Records one flip of the Rumble Pak's register.
        void RumbleStart() noexcept { ++_edges; }

        /// Does nothing.
        ///
        /// melonDS calls \c Platform::Addon_RumbleStop immediately before
        /// \c Platform::Addon_RumbleStart on every register flip and never on its own
        /// (see \c CartRumblePak::ROMWrite in \c src/GBACart.cpp, as of melonDS 906e9ebb),
        /// so counting both would count every toggle twice.
        /// If a future melonDS breaks that pairing, this is where to fix it.
        void RumbleStop() noexcept {}

        /// Folds this frame's register toggles into the motors' strength.
        ///
        /// Call once per emulated frame, after the frame has been run.
        void Update() noexcept;

        /// Switches the motors off and forgets the buffered toggle history.
        void Stop() noexcept;

        /// The strength most recently computed for the motors.
        [[nodiscard]] uint16_t Level() const noexcept { return _level; }

        /// The number of register toggles counted in the most recent frame.
        [[nodiscard]] uint32_t Edges() const noexcept { return _lastEdges; }

    private:
        void Emit(uint16_t strength) const noexcept;

        RumbleEnvelope _envelope;
        uint32_t _edges = 0;
        uint32_t _lastEdges = 0;
        uint16_t _level = 0;
        uint16_t _lastStrength = 0;
        uint16_t _intensity = UINT16_MAX;
        RumbleMotorType _motors = RumbleMotorType::Both;
        bool _armed = true;
    };
}
