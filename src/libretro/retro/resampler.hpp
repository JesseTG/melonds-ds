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

#ifndef MELONDS_DS_RESAMPLER_HPP
#define MELONDS_DS_RESAMPLER_HPP

#include <cstdint>
#include <optional>
#include <vector>

#include <audio/audio_resampler.h>

#include "std/span.hpp"

namespace retro {
    /// Converts a stream of audio samples from one sample rate to another.
    ///
    /// This is a thin wrapper around libretro-common's resampler interface;
    /// it exposes only the parts that melonDS DS uses.
    class Resampler {
    public:
        /// Creates a resampler that converts from \c inputRate to \c outputRate.
        /// \returns \c std::nullopt if libretro-common couldn't provide a resampler.
        static std::optional<Resampler> Create(
            double inputRate,
            double outputRate,
            resampler_quality quality = RESAMPLER_QUALITY_NORMAL
        ) noexcept;

        ~Resampler() noexcept;
        Resampler(const Resampler&) = delete;
        Resampler& operator=(const Resampler&) = delete;
        Resampler(Resampler&&) noexcept;
        Resampler& operator=(Resampler&&) noexcept;

        /// The number of output samples produced per input sample.
        [[nodiscard]] double Ratio() const noexcept { return _ratio; }

        /// Resamples a block of monaural samples.
        ///
        /// libretro-common's resamplers all operate on stereo frames,
        /// so the input is duplicated to both channels
        /// and only the left channel of the result is kept.
        ///
        /// \param in The samples to resample.
        /// \param out Where the resampled audio will be written.
        /// Excess output is discarded, so this should hold
        /// at least <tt>in.size() * Ratio()</tt> samples.
        /// \returns The number of samples written to \c out.
        size_t ProcessMono(std::span<const int16_t> in, std::span<int16_t> out) noexcept;

    private:
        Resampler(void* handle, const retro_resampler_t* backend, double ratio) noexcept;

        void* _handle;
        const retro_resampler_t* _backend;
        double _ratio;

        // Scratch space, kept around so that each call doesn't have to reallocate it
        std::vector<float> _mono;
        std::vector<float> _stereoIn;
        std::vector<float> _stereoOut;
    };
}

#endif // MELONDS_DS_RESAMPLER_HPP
