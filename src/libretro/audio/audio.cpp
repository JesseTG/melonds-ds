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

#include "audio.hpp"

#include <algorithm>
#include <new>
#include <optional>

#include <NDS.h>
#include <libretro.h>

#include "../config/config.hpp"
#include "../constants.hpp"
#include "../environment.hpp"
#include "../tracy.hpp"

using namespace MelonDsDs;

MelonDsDs::AudioState::AudioState() noexcept {
    _lowPass.Init(SAMPLE_RATE);
}

void MelonDsDs::AudioState::SetConfig(const CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);

    _lowPassReference = config.SpeedUpLowPass();
    bool enabled = config.TimeStretch();

    // A few hundred KB of rings, and CoreState lives in a static buffer sized by
    // sizeof(CoreState); allocate on demand so it costs nothing when unused.
    if (enabled && !_stretch) {
        _stretch.reset(new(std::nothrow) AudioTimeStretch());
        if (!_stretch)
            retro::warn("Couldn't allocate the audio time-stretcher; fast-forward audio will not be smoothed");
    } else if (!enabled && _stretch) {
        _stretch.reset();
    }

    _timeStretchEnabled = enabled;
    if (!Enabled()) _engaged = false;
}

void MelonDsDs::AudioState::Reset() noexcept {
    ZoneScopedN(TracyFunction);

    if (_stretch) _stretch->Reset();
    _lowPass.Init(SAMPLE_RATE);
    _engaged = false;
    _outputCredit = 0.0;
    _arrivalAvg = 0.0;
    _outputAvg = 0.0;
    _lastRender = std::chrono::steady_clock::time_point {};
}

MelonDsDs::AudioState::Throttle MelonDsDs::AudioState::QueryThrottle() noexcept {
    if (std::optional<retro_throttle_state> throttle = retro::get_throttle_state()) {
        switch (throttle->mode) {
            case RETRO_THROTTLE_FAST_FORWARD:
            case RETRO_THROTTLE_SLOW_MOTION:
                // rate is documented as inaccurate when the core can't keep up, and
                // zero when there's no fixed target, so it only seeds the estimate.
                return {
                    ThrottleClass::OffSpeed,
                    throttle->rate > 0.0f ? throttle->rate / FPS : 0.0
                };

            case RETRO_THROTTLE_UNBLOCKED:
                // Not necessarily fast: also reported with vsync and audio sync off,
                // where the frame limiter may still hold 1x. Decide by measurement.
                return {
                    ThrottleClass::Unthrottled,
                    throttle->rate > 0.0f ? throttle->rate / FPS : 0.0
                };

            case RETRO_THROTTLE_REWINDING:
            case RETRO_THROTTLE_FRAME_STEPPING:
                // The frontend reorders or withholds frames and handles audio itself.
                return {ThrottleClass::Passthrough, 0.0};

            default: // NONE, VSYNC
                return {ThrottleClass::Normal, 1.0};
        }
    }

    // No throttle interface; the coarser query says whether but not by how much,
    // which is enough because the ratio is measured anyway.
    if (std::optional<bool> fastforwarding = retro::is_fastforwarding(); fastforwarding && *fastforwarding)
        return {ThrottleClass::OffSpeed, 0.0};

    return {ThrottleClass::Normal, 1.0};
}

void MelonDsDs::AudioState::Render(melonDS::NDS& nds) noexcept {
    ZoneScopedN(TracyFunction);

    int available = std::min(nds.SPU.GetOutputSize(), RENDER_CAPACITY);
    int arrived = (available > 0) ? static_cast<int>(nds.SPU.ReadOutput(_buffer, available)) : 0;

    // Advances on every call, engaged or not, so the speed estimate that decides
    // whether to engage has settled by the time we need it.
    auto now = std::chrono::steady_clock::now();
    double elapsed;
    if (_lastRender == std::chrono::steady_clock::time_point {}) {
        // First call after a reset; the epoch would read as decades of elapsed time.
        elapsed = 1.0 / FPS;
    } else {
        elapsed = std::chrono::duration<double>(now - _lastRender).count();
        if (!(elapsed > 0.0)) elapsed = 0.0;
        if (elapsed > MAX_ELAPSED_SECONDS) elapsed = MAX_ELAPSED_SECONDS;
    }
    _lastRender = now;

    // Frames real time owes the frontend for this call.
    double realtime = elapsed * SAMPLE_RATE;

    _arrivalAvg += (arrived - _arrivalAvg) * RATE_SMOOTHING;
    _outputAvg += (realtime - _outputAvg) * RATE_SMOOTHING;

    Throttle throttle = QueryThrottle();

    bool stretch = Enabled() && (
        throttle.cls == ThrottleClass::OffSpeed ||
        (throttle.cls == ThrottleClass::Unthrottled && AudioIsOffSpeed(MeasuredSpeed()))
    );

    if (!stretch) {
        _engaged = false;
        PushDirect(arrived);
        return;
    }

    if (!_engaged) Engage(throttle.hint);

    _outputCredit += realtime;
    PushStretched(arrived);
}

double MelonDsDs::AudioState::MeasuredSpeed() const noexcept {
    // Guarded: _outputAvg starts at zero and takes a few calls to mean anything.
    return (_outputAvg > 1.0) ? (_arrivalAvg / _outputAvg) : 1.0;
}

void MelonDsDs::AudioState::Engage(double hint) noexcept {
    _engaged = true;
    _outputCredit = 0.0;
    _stretch->BeginSession();

    // Seed from the frontend's hint so the first calls stretch by roughly the right
    // amount instead of ramping from 1x; measurement overrides it either way.
    if (hint > 0.0) {
        double perCall = SAMPLE_RATE / FPS;
        _arrivalAvg = perCall;
        _outputAvg = perCall / hint;
    }
}

int MelonDsDs::AudioState::PaceOutput() noexcept {
    int frames = static_cast<int>(_outputCredit);
    if (frames < 0) frames = 0;
    if (frames > RENDER_CAPACITY) frames = RENDER_CAPACITY;
    _outputCredit -= frames;
    return frames;
}

void MelonDsDs::AudioState::PushDirect(int frames) noexcept {
    if (frames <= 0) return;

    // Speed 1.0 leaves the filter wide open, but it still runs so its state stays
    // in step with the signal.
    ApplyLowPass(frames, 1.0);
    retro::audio_sample_batch(_buffer, frames);
}

void MelonDsDs::AudioState::PushStretched(int arrived) noexcept {
    // Frames are already out of the SPU FIFO, so a short accept loses them; mark
    // the discontinuity so the stretcher resyncs rather than splicing.
    if (arrived > 0 && _stretch->Write(_buffer, arrived) < arrived)
        _stretch->BeginSession();

    int wanted = PaceOutput();
    if (wanted <= 0) return;

    double ratio = AudioComputeStretchRatio(
        _arrivalAvg,
        _outputAvg,
        _stretch->InputFill(),
        AudioTimeStretch::TargetInputFill(_arrivalAvg)
    );

    int produced = _stretch->Read(_buffer, wanted, ratio);

    // Push mode needn't fill a buffer, so a shortfall needs no padding - but it must
    // be given back, or the output rate sits below real time and underruns forever.
    if (produced < wanted) _outputCredit += (wanted - produced);
    if (produced <= 0) return;

    ApplyLowPass(produced, MeasuredSpeed());
    retro::audio_sample_batch(_buffer, produced);
}

void MelonDsDs::AudioState::ApplyLowPass(int frames, double speed) noexcept {
    // Skip the biquads outright when the filter can never engage.
    if (!_timeStretchEnabled || _lowPassReference >= SPEEDUP_LOWPASS_OFF) return;

    double cutoff = AudioComputeLowPassCutoff(speed, _lowPassReference, _lowPass.WideOpenCutoff());
    _lowPass.Process(_buffer, frames, cutoff, frames / SAMPLE_RATE);
}
