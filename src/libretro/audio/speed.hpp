/*
    Copyright 2026 Davey Hughes

    Ported from the melonDS standalone frontend's AudioSpeed.h. Its output-skew
    helper is omitted; this core always emits at the DS's native rate.

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

#ifndef MELONDS_DS_AUDIO_SPEED_HPP
#define MELONDS_DS_AUDIO_SPEED_HPP

#include <algorithm>
#include <cmath>

namespace MelonDsDs {
    /// Sentinel cutoff meaning "never filter"; a merely high value would still
    /// bite once divided by the speed.
    constexpr int SPEEDUP_LOWPASS_OFF = 24000;

    /// Default cutoff. Transparent until about 1.4x, since the DS's output rate puts
    /// wide open at ~14.7 kHz.
    constexpr int SPEEDUP_LOWPASS_DEFAULT = 20000;

    /// Gain of the fill correction on the stretch ratio.
    constexpr double STRETCH_TRIM_GAIN = 0.25;

    /// Below 1 the stretcher expands (slow-mo), above it compresses (fast-forward).
    constexpr double MIN_STRETCH_RATIO = 0.25;
    constexpr double MAX_STRETCH_RATIO = 32.0;

    /// A speed far enough from 1.0 to be worth stretching for.
    constexpr double SPEED_DEADZONE = 0.01;

    [[nodiscard]] inline bool AudioIsOffSpeed(double speed) noexcept {
        return std::fabs(speed - 1.0) > SPEED_DEADZONE;
    }

    /// How much input each output frame consumes. Driven by measured arrival rather
    /// than the frontend's claimed speed, which is often wrong: in steady state
    /// consumption equals arrival. The fill term only steers the buffer level.
    [[nodiscard]] inline double AudioComputeStretchRatio(
        double arrivalPerCall,
        double outputPerCall,
        int inputFill,
        int targetFill
    ) noexcept {
        if (outputPerCall <= 0.0) return 1.0;

        double ratio = arrivalPerCall / outputPerCall;

        if (targetFill > 0) {
            double err = (inputFill - static_cast<double>(targetFill)) / static_cast<double>(targetFill);
            err = std::clamp(err, -1.0, 1.0);
            ratio *= 1.0 + (STRETCH_TRIM_GAIN * err);
        }

        return std::clamp(ratio, MIN_STRETCH_RATIO, MAX_STRETCH_RATIO);
    }

    /// Low-pass cutoff in Hz. Engages only above normal speed, and gets duller the
    /// faster the emulator runs.
    [[nodiscard]] inline double AudioComputeLowPassCutoff(
        double speed,
        int reference,
        double wideOpen
    ) noexcept {
        if (wideOpen <= 200.0) return wideOpen;
        if (speed <= 1.0) return wideOpen;
        if (reference >= SPEEDUP_LOWPASS_OFF) return wideOpen;

        return std::clamp(reference / speed, 200.0, wideOpen);
    }
}

#endif //MELONDS_DS_AUDIO_SPEED_HPP
