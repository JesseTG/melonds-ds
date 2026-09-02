/*
    Copyright 2026 Davey Hughes

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

#ifndef MELONDS_DS_AUDIO_STATE_HPP
#define MELONDS_DS_AUDIO_STATE_HPP

#include <chrono>
#include <cstdint>
#include <memory>

#include "lowpass.hpp"
#include "speed.hpp"
#include "timestretch.hpp"

namespace melonDS {
    class NDS;
}

namespace MelonDsDs {
    class CoreConfig;

    /// Owns everything between the SPU and the frontend's audio callback.
    ///
    /// At normal speed this is a passthrough. Off-speed the SPU still produces one
    /// frame of audio per retro_run(), but those calls no longer arrive at real time,
    /// so the stretcher rescales the stream to real time while preserving pitch.
    class AudioState {
    public:
        AudioState() noexcept;

        void SetConfig(const CoreConfig& config) noexcept;

        /// Drops buffered audio; call on console reset and savestate load.
        void Reset() noexcept;

        /// Drains the SPU and pushes one call's worth of audio to the frontend.
        [[gnu::hot]] void Render(melonDS::NDS& nds) noexcept;

    private:
        /// Bounds what one Read can return. Bigger than a DS frame's worth of audio
        /// because slow motion owes ~2188 frames per call at 0.25x.
        static constexpr int RENDER_CAPACITY = AudioTimeStretch::OUTPUT_CAPACITY;

        static constexpr double RATE_SMOOTHING = 0.05;

        /// Longest gap the pacer honours; a pause would otherwise bank seconds of
        /// credit and discharge it as one burst.
        static constexpr double MAX_ELAPSED_SECONDS = 0.1;

        enum class ThrottleClass {
            Normal,      ///< running us at the DS's own rate
            OffSpeed,    ///< fast-forward or slow motion, stated outright
            Unthrottled, ///< blocking on nothing; only measurement can say the speed
            Passthrough, ///< the frontend owns the audio (rewind, frame stepping)
        };

        struct Throttle {
            ThrottleClass cls;
            double hint;  ///< speed estimate, or 0 when the frontend can't say
        };

        [[nodiscard]] static Throttle QueryThrottle() noexcept;

        [[nodiscard]] bool Enabled() const noexcept { return _timeStretchEnabled && _stretch != nullptr; }

        /// Emulated speed relative to real time, measured rather than claimed.
        [[nodiscard]] double MeasuredSpeed() const noexcept;

        void Engage(double hint) noexcept;
        [[nodiscard]] int PaceOutput() noexcept;
        void PushDirect(int frames) noexcept;
        void PushStretched(int arrived) noexcept;
        void ApplyLowPass(int frames, double speed) noexcept;

        std::unique_ptr<AudioTimeStretch> _stretch = nullptr;
        AudioLowPass _lowPass {};
        std::chrono::steady_clock::time_point _lastRender {};

        /// Fractional real-time frames owed to the frontend. Integrating rather than
        /// rounding per call keeps the long-run output rate exactly real time.
        double _outputCredit = 0.0;

        double _arrivalAvg = 0.0;
        double _outputAvg = 0.0;
        int _lowPassReference = SPEEDUP_LOWPASS_OFF;
        bool _timeStretchEnabled = false;
        bool _engaged = false;
        int16_t _buffer[RENDER_CAPACITY * 2] {};
    };
}

#endif //MELONDS_DS_AUDIO_STATE_HPP
