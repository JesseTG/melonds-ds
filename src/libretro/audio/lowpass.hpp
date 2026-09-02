/*
    Copyright 2026 Davey Hughes

    Ported from the melonDS standalone frontend's AudioLowPass.h.

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

#ifndef MELONDS_DS_AUDIO_LOWPASS_HPP
#define MELONDS_DS_AUDIO_LOWPASS_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace MelonDsDs {
    /// Fourth-order Butterworth low-pass: two cascaded RBJ biquads, stereo with
    /// per-channel state. The cutoff is smoothed rather than stepped, which would click.
    class AudioLowPass {
    public:
        /// Time constant of the cutoff smoother, in seconds.
        static constexpr double SMOOTHING_TAU = 0.05;

        /// Fraction of wide-open at which the filter stops touching the output.
        static constexpr double BYPASS_THRESHOLD = 0.995;

        /// Section Q's for a fourth-order Butterworth cascade.
        static constexpr double SECTION_Q[2] = {0.54119610014619698, 1.3065629648763766};

        /// Lowest cutoff the coefficient design will accept.
        static constexpr double MIN_CUTOFF = 20.0;

        void Init(double sampleRate) noexcept {
            _sampleRate = sampleRate;
            _wideOpen = std::max(0.45 * sampleRate, MIN_CUTOFF);
            for (int s = 0; s < 2; s++) {
                _stages[s].z1[0] = _stages[s].z1[1] = 0.0;
                _stages[s].z2[0] = _stages[s].z2[1] = 0.0;
            }
            SetCutoffNow(_wideOpen);
        }

        [[nodiscard]] double WideOpenCutoff() const noexcept { return _wideOpen; }
        [[nodiscard]] double Cutoff() const noexcept { return _curCutoff; }
        [[nodiscard]] bool Bypassed() const noexcept { return _curCutoff >= (_wideOpen * BYPASS_THRESHOLD); }

        /// Advance the smoothed cutoff by one block, then filter in place.
        void Process(int16_t* samples, int numFrames, double targetHz, double blockSeconds) noexcept {
            Smooth(targetHz, blockSeconds);
            bool bypass = Bypassed();

            for (int i = 0; i < numFrames; i++) {
                for (int ch = 0; ch < 2; ch++) {
                    // Runs even when bypassed, to keep the state in step with the signal.
                    double y = ProcessSample(samples[(i * 2) + ch], ch);
                    if (!bypass) samples[(i * 2) + ch] = Saturate(y);
                }
            }
        }

        void Smooth(double targetHz, double blockSeconds) noexcept {
            targetHz = std::clamp(targetHz, MIN_CUTOFF, _wideOpen);
            double a = 1.0 - std::exp(-blockSeconds / SMOOTHING_TAU);
            SetCutoffNow(_curCutoff + ((targetHz - _curCutoff) * a));
        }

        void SetCutoffNow(double cutoffHz) noexcept {
            _curCutoff = std::clamp(cutoffHz, MIN_CUTOFF, _wideOpen);
            for (int s = 0; s < 2; s++)
                _stages[s].Design(_curCutoff, _sampleRate, SECTION_Q[s]);
        }

        double ProcessSample(double x, int ch) noexcept {
            double y = x;
            for (int s = 0; s < 2; s++)
                y = _stages[s].Run(y, ch);
            return y;
        }

    private:
        static constexpr double PI = 3.14159265358979323846;

        static int16_t Saturate(double y) noexcept {
            long v = std::lround(y);
            if (v > 32767) v = 32767;
            if (v < -32768) v = -32768;
            return static_cast<int16_t>(v);
        }

        struct Biquad {
            double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
            double z1[2] = {0.0, 0.0};
            double z2[2] = {0.0, 0.0};

            void Design(double cutoffHz, double sampleRate, double q) noexcept {
                double w0 = 2.0 * PI * (cutoffHz / sampleRate);
                double cw = std::cos(w0);
                double alpha = std::sin(w0) / (2.0 * q);
                double a0 = 1.0 + alpha;

                b0 = ((1.0 - cw) * 0.5) / a0;
                b1 = (1.0 - cw) / a0;
                b2 = b0;
                a1 = (-2.0 * cw) / a0;
                a2 = (1.0 - alpha) / a0;
            }

            /// transposed direct form II
            double Run(double x, int ch) noexcept {
                double y = (b0 * x) + z1[ch];
                z1[ch] = (b1 * x) - (a1 * y) + z2[ch];
                z2[ch] = (b2 * x) - (a2 * y);
                return y;
            }
        };

        double _sampleRate = 32728.5;
        double _wideOpen = 14727.8;
        double _curCutoff = 14727.8;
        Biquad _stages[2];
    };
}

#endif //MELONDS_DS_AUDIO_LOWPASS_HPP
