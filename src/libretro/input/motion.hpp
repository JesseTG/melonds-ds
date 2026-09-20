/*
    Copyright 2026 Jesse Talavera

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

#include <cstdint>

#include <glm/vec3.hpp>
#include <libretro.h>
#include <Platform.h>

namespace MelonDsDs {
    class CoreConfig;

    /// Standard gravity, in m/s^2.
    /// libretro reports acceleration in multiples of this,
    /// but melonDS expects it in m/s^2.
    constexpr float STANDARD_GRAVITY = 9.80665f;

    /// Feeds the frontend's accelerometer and gyroscope to an emulated DS Motion Pak.
    ///
    /// Owns the frontend's motion sensors for as long as a Motion Pak is in Slot-2;
    /// they're switched off when this object is destroyed or moved from.
    /// Any sensor the frontend can't provide
    /// reads as if the console were lying flat and still.
    ///
    /// libretro's sensor axes are the same as melonDS's
    /// (X to the right, Y up, and Z toward the viewer),
    /// so the frontend's readings pass straight through
    /// apart from converting the units.
    class MotionState {
    public:
        /// \param port The frontend port whose sensors will be used.
        /// \param hasGyroscope Whether the emulated pak has a gyroscope.
        /// Only the homebrew Motion Pak does;
        /// melonDS never asks the retail Motion Pack for rotation.
        MotionState(unsigned port, bool hasGyroscope) noexcept;
        ~MotionState() noexcept;
        MotionState(const MotionState&) = delete;
        MotionState& operator=(const MotionState&) = delete;
        MotionState(MotionState&&) noexcept;
        MotionState& operator=(MotionState&&) noexcept;

        void SetConfig(const CoreConfig& config) noexcept;

        /// Reads the frontend's motion sensors.
        ///
        /// Call once per frame, after polling for input.
        void Update() noexcept;

        /// The reading that melonDS asked for,
        /// in m/s^2 for acceleration and rad/s for rotation.
        [[nodiscard]] float Query(melonDS::Platform::MotionQueryType type) const noexcept;

        /// What a Motion Pak reads while lying flat and still:
        /// 1g upwards along its Z axis, and nothing else.
        [[nodiscard]] static float Resting(melonDS::Platform::MotionQueryType type) noexcept;

    private:
        enum class InterfaceState : uint8_t {
            /// Never requested, or already released.
            Off,
            /// The frontend refused this sensor twice, so it won't be asked again.
            Unavailable,
            /// The frontend refused this sensor once;
            /// it'll be asked again in the next \c Update.
            Deferred,
            On,
        };

        /// Asks the frontend to switch a sensor on.
        /// \return \c On if it did, otherwise \c failure.
        [[nodiscard]] InterfaceState Enable(retro_sensor_action action, const char* name, InterfaceState failure) const noexcept;

        /// Switches a sensor off if it's on, and forgets about it either way.
        void Disable(InterfaceState& state, retro_sensor_action action, const char* name) const noexcept;

        unsigned _port;
        InterfaceState _accelerometer = InterfaceState::Off;
        InterfaceState _gyroscope = InterfaceState::Off;
        bool _showWarnings = true;

        /// The most recent acceleration, in g.
        glm::vec3 _acceleration {0.0f, 0.0f, 1.0f};

        /// The most recent angular velocity, in rad/s.
        glm::vec3 _rotation {0.0f, 0.0f, 0.0f};
    };
}
