/*
    Copyright 2026 Davey Hughes

    Ported from the melonDS standalone frontend's AudioTimeStretch.h. The original
    is lock-free SPSC because an emu thread fills it and an SDL audio thread drains
    it; here everything runs on one thread, so Write and Read are strictly ordered
    and the cross-thread machinery collapses into plain members.

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

#ifndef MELONDS_DS_AUDIO_TIMESTRETCH_HPP
#define MELONDS_DS_AUDIO_TIMESTRETCH_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace MelonDsDs {
    /// WSOLA time-stretcher: changes playback rate while preserving pitch by
    /// overlap-adding windowed frames picked for waveform similarity.
    ///
    /// FRAME_SIZE and SEARCH_RADIUS are tuned, not guessed. Amplitude pumping at
    /// ratio 3, search on/off: frame 1024 = 0.139/0.180, 512 = 0.071/0.175,
    /// 256 = 0.036/0.169. Below 256 the window holds barely a cycle of a bass note.
    class AudioTimeStretch {
    public:
        static constexpr int FRAME_SIZE = 256;                 ///< analysis window, frames
        static constexpr int SYNTHESIS_HOP = FRAME_SIZE / 2;   ///< periodic Hann at 50% sums to unity
        static constexpr int SEARCH_RADIUS = 1024;             ///< frames either side of nominal
        static constexpr int COARSE_STRIDE = 4;
        static constexpr int FINE_RADIUS = 3;
        static constexpr int INPUT_CAPACITY = 32768;           ///< must be a power of two
        static constexpr int OUTPUT_CAPACITY = 8192;           ///< must be a power of two

        /// Floor on the buffered input the ratio control aims for.
        static constexpr int MIN_TARGET_INPUT_FILL = 4096;

        /// How much input to keep buffered. A fixed figure starves the stretcher at
        /// high speeds, where one call can consume more than the whole target.
        static int TargetInputFill(double arrivalPerCall) noexcept {
            // Half the ring; above a ratio of ~9 the floor follows _naturalPos, whose
            // span grows with the hop, which would otherwise squeeze Write to nothing.
            const double cap = INPUT_CAPACITY / 2;

            double need = (2.0 * arrivalPerCall) + SEARCH_RADIUS + FRAME_SIZE;
            // Clamped as a double; a wild estimate is UB once cast to int.
            if (!(need > MIN_TARGET_INPUT_FILL)) need = MIN_TARGET_INPUT_FILL;
            if (need > cap) need = cap;
            return static_cast<int>(need);
        }

        AudioTimeStretch() noexcept {
            constexpr double pi = 3.14159265358979323846;
            for (int i = 0; i < FRAME_SIZE; i++)
                _window[i] = static_cast<float>(0.5 * (1.0 - std::cos((2.0 * pi * i) / FRAME_SIZE)));
            Reset();
        }

        void Reset() noexcept {
            _writePos = 0;
            _resyncPending = false;
            _analysisPos = 0;
            _naturalPos = 0;
            _outReadPos = 0;
            _outWritePos = 0;
            _primed = false;
            std::memset(_accL, 0, sizeof(_accL));
            std::memset(_accR, 0, sizeof(_accR));

            // Belt and braces, so any future bound slip degrades to silence, not noise.
            std::memset(_inL, 0, sizeof(_inL));
            std::memset(_inR, 0, sizeof(_inR));
            std::memset(_inMono, 0, sizeof(_inMono));
        }

        [[nodiscard]] int InputFill() const noexcept {
            int64_t pending = _writePos - _analysisPos;
            if (pending < 0) return 0;
            if (pending > INPUT_CAPACITY) return INPUT_CAPACITY;
            return static_cast<int>(pending);
        }

        [[nodiscard]] int OutputFill() const noexcept { return static_cast<int>(_outWritePos - _outReadPos); }

        /// Total frames written since the last Reset.
        [[nodiscard]] int64_t TotalWritten() const noexcept { return _writePos; }

        /// Append interleaved stereo frames, returning how many were accepted. Writing
        /// is refused rather than allowed to lap the frames the search may still touch.
        int Write(const int16_t* samples, int numFrames) noexcept {
            int64_t space = INPUT_CAPACITY - (_writePos - OldestNeeded());
            if (space < 0) space = 0;
            if (numFrames > space) numFrames = static_cast<int>(space);
            if (numFrames <= 0) return 0;

            for (int i = 0; i < numFrames; i++) {
                int idx = static_cast<int>((_writePos + i) & (INPUT_CAPACITY - 1));
                int16_t l = samples[(i * 2) + 0];
                int16_t r = samples[(i * 2) + 1];
                _inL[idx] = l;
                _inR[idx] = r;
                _inMono[idx] = 0.5f * (static_cast<float>(l) + static_cast<float>(r));
            }

            _writePos += numFrames;
            return numFrames;
        }

        /// Marks a discontinuity, so the next Read resyncs to live data instead of
        /// splicing across the gap. Deferred because the new data isn't written yet.
        void BeginSession() noexcept { _resyncPending = true; }

        /// Emit up to numFrames of interleaved stereo, synthesising as needed.
        int Read(int16_t* samples, int numFrames, double ratio) noexcept {
            if (_resyncPending) {
                _resyncPending = false;
                ResyncToLive();
            }

            // Write refuses to lap the synthesis, so this shouldn't trigger; if a
            // future change lets it, skip forward rather than read overwritten frames.
            int64_t floor = (_writePos - INPUT_CAPACITY) + SEARCH_RADIUS + FRAME_SIZE;
            if (_analysisPos < floor) {
                _analysisPos = floor;
                _naturalPos = floor;
                _primed = false;
            }

            while ((OutputFill() < numFrames) && CanSynthesise())
                SynthesiseHop(ratio);

            int n = std::min(numFrames, OutputFill());
            for (int i = 0; i < n; i++) {
                int idx = static_cast<int>(_outReadPos & (OUTPUT_CAPACITY - 1));
                samples[(i * 2) + 0] = _outL[idx];
                samples[(i * 2) + 1] = _outR[idx];
                _outReadPos++;
            }

            return n;
        }

    private:
        static int16_t Saturate(float v) noexcept {
            long s = std::lround(v);
            if (s > 32767) s = 32767;
            if (s < -32768) s = -32768;
            return static_cast<int16_t>(s);
        }

        /// Oldest frame the synthesis might still touch. _naturalPos belongs here too:
        /// at a large hop it sits well below _analysisPos - SEARCH_RADIUS.
        [[nodiscard]] int64_t OldestNeeded() const noexcept {
            int64_t floor = std::min(_analysisPos - SEARCH_RADIUS, _naturalPos);
            return floor < 0 ? 0 : floor;
        }

        /// Drop whatever is stale and pick up at live data.
        void ResyncToLive() noexcept {
            _analysisPos = std::max<int64_t>(0, _writePos - FRAME_SIZE);
            _naturalPos = _analysisPos;
            _primed = false;
            _outReadPos = 0;
            _outWritePos = 0;
            std::memset(_accL, 0, sizeof(_accL));
            std::memset(_accR, 0, sizeof(_accR));
        }

        [[nodiscard]] bool CanSynthesise() const noexcept {
            if ((OUTPUT_CAPACITY - OutputFill()) < SYNTHESIS_HOP) return false;

            // Only the frame and the continuation reference need be present; demanding
            // the full radius would emit nothing on a short FIFO.
            int64_t frameEnd = _analysisPos + FRAME_SIZE;
            int64_t naturalEnd = _naturalPos + SYNTHESIS_HOP;
            return (_writePos >= frameEnd) && (_writePos >= naturalEnd);
        }

        void SynthesiseHop(double ratio) noexcept {
            int hop = static_cast<int>(std::lround(SYNTHESIS_HOP * ratio));
            if (hop < 1) hop = 1;

            int64_t chosen = _primed ? FindBestOffset() : _analysisPos;
            _primed = true;

            for (int i = 0; i < FRAME_SIZE; i++) {
                int idx = static_cast<int>((chosen + i) & (INPUT_CAPACITY - 1));
                float w = _window[i];
                _accL[i] += w * static_cast<float>(_inL[idx]);
                _accR[i] += w * static_cast<float>(_inR[idx]);
            }

            for (int i = 0; i < SYNTHESIS_HOP; i++) {
                int idx = static_cast<int>(_outWritePos & (OUTPUT_CAPACITY - 1));
                _outL[idx] = Saturate(_accL[i]);
                _outR[idx] = Saturate(_accR[i]);
                _outWritePos++;
            }

            std::memmove(_accL, _accL + SYNTHESIS_HOP, SYNTHESIS_HOP * sizeof(float));
            std::memmove(_accR, _accR + SYNTHESIS_HOP, SYNTHESIS_HOP * sizeof(float));
            std::memset(_accL + SYNTHESIS_HOP, 0, SYNTHESIS_HOP * sizeof(float));
            std::memset(_accR + SYNTHESIS_HOP, 0, SYNTHESIS_HOP * sizeof(float));

            // Advances by hop alone; advancing from chosen would make consumption
            // hop + E[bestK], which the caller's ratio control cannot see.
            _naturalPos = chosen + SYNTHESIS_HOP;
            _analysisPos += hop;
        }

        [[nodiscard]] int64_t FindBestOffset() const noexcept {
            int64_t oldest = std::max<int64_t>(0, _writePos - INPUT_CAPACITY);

            // _naturalPos can fall off the back at a high ratio on a near-full ring;
            // the reference would then be overwritten frames, so don't search.
            if (_naturalPos < oldest) return _analysisPos;

            double refEnergy = Energy(_naturalPos);

            // Full radius regardless of hop; expansion needs the reach.
            const int radius = SEARCH_RADIUS;

            // Masking a negative position wraps it into frames we never wrote.
            int lowestK = -radius;
            if ((_analysisPos + lowestK) < oldest)
                lowestK = static_cast<int>(oldest - _analysisPos);

            // Clamp to what has arrived, so a short FIFO narrows the search.
            int highestK = radius;
            int64_t latest = _writePos - FRAME_SIZE;
            if ((_analysisPos + highestK) > latest)
                highestK = static_cast<int>(latest - _analysisPos);
            if (highestK < lowestK) highestK = lowestK;

            int bestK = std::min(std::max(lowestK, 0), highestK);
            double bestScore = -1.0e30;

            for (int k = lowestK; k <= highestK; k += COARSE_STRIDE) {
                double s = Score(_analysisPos + k, refEnergy);
                if (s > bestScore) { bestScore = s; bestK = k; }
            }

            int lo = std::max(lowestK, bestK - FINE_RADIUS);
            int hi = std::min(highestK, bestK + FINE_RADIUS);
            for (int k = lo; k <= hi; k++) {
                double s = Score(_analysisPos + k, refEnergy);
                if (s > bestScore) { bestScore = s; bestK = k; }
            }

            return _analysisPos + bestK;
        }

        [[nodiscard]] double Energy(int64_t pos) const noexcept {
            double e = 0.0;
            for (int i = 0; i < SYNTHESIS_HOP; i++) {
                double v = _inMono[static_cast<int>((pos + i) & (INPUT_CAPACITY - 1))];
                e += v * v;
            }
            return e;
        }

        /// Normalised so the search doesn't just latch onto the loudest candidate.
        [[nodiscard]] double Score(int64_t pos, double refEnergy) const noexcept {
            double dot = 0.0;
            double energy = 0.0;
            for (int i = 0; i < SYNTHESIS_HOP; i++) {
                double a = _inMono[static_cast<int>((pos + i) & (INPUT_CAPACITY - 1))];
                double b = _inMono[static_cast<int>((_naturalPos + i) & (INPUT_CAPACITY - 1))];
                dot += a * b;
                energy += a * a;
            }
            return dot / std::sqrt((energy * refEnergy) + 1.0e-9);
        }

        float _window[FRAME_SIZE];

        int16_t _inL[INPUT_CAPACITY];
        int16_t _inR[INPUT_CAPACITY];
        float _inMono[INPUT_CAPACITY];
        int64_t _writePos = 0;

        bool _resyncPending = false;
        int64_t _analysisPos = 0;
        int64_t _naturalPos = 0;
        bool _primed = false;

        float _accL[FRAME_SIZE];
        float _accR[FRAME_SIZE];

        int16_t _outL[OUTPUT_CAPACITY];
        int16_t _outR[OUTPUT_CAPACITY];
        int64_t _outReadPos = 0;
        int64_t _outWritePos = 0;
    };
}

#endif //MELONDS_DS_AUDIO_TIMESTRETCH_HPP
