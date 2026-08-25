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

#include "resampler.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include <audio/conversion/float_to_s16.h>
#include <audio/conversion/s16_to_float.h>

#include "../tracy.hpp"

using std::optional;
using std::nullopt;

/// Extra frames to allocate for the resampler's output,
/// since it decides for itself how many frames a given block produces.
constexpr size_t OUTPUT_SLACK = 16;

optional<retro::Resampler> retro::Resampler::Create(
    double inputRate,
    double outputRate,
    resampler_quality quality
) noexcept {
    if (inputRate <= 0.0 || outputRate <= 0.0)
        return nullopt;

    double ratio = outputRate / inputRate;
    void* handle = nullptr;
    const retro_resampler_t* backend = nullptr;

    // A null identifier means "use the first available resampler".
    if (!retro_resampler_realloc(&handle, &backend, nullptr, quality, ratio))
        return nullopt;

    return Resampler(handle, backend, ratio);
}

retro::Resampler::Resampler(void* handle, const retro_resampler_t* backend, double ratio) noexcept :
    _handle(handle),
    _backend(backend),
    _ratio(ratio) {
}

retro::Resampler::~Resampler() noexcept {
    if (_handle && _backend && _backend->free) {
        _backend->free(_handle);
    }
}

retro::Resampler::Resampler(Resampler&& other) noexcept :
    _handle(std::exchange(other._handle, nullptr)),
    _backend(std::exchange(other._backend, nullptr)),
    _ratio(std::exchange(other._ratio, 1.0)),
    _mono(std::move(other._mono)),
    _stereoIn(std::move(other._stereoIn)),
    _stereoOut(std::move(other._stereoOut)) {
}

retro::Resampler& retro::Resampler::operator=(Resampler&& other) noexcept {
    if (this != &other) {
        if (_handle && _backend && _backend->free) {
            _backend->free(_handle);
        }

        _handle = std::exchange(other._handle, nullptr);
        _backend = std::exchange(other._backend, nullptr);
        _ratio = std::exchange(other._ratio, 1.0);
        _mono = std::move(other._mono);
        _stereoIn = std::move(other._stereoIn);
        _stereoOut = std::move(other._stereoOut);
    }

    return *this;
}

size_t retro::Resampler::ProcessMono(std::span<const int16_t> in, std::span<int16_t> out) noexcept {
    ZoneScopedN(TracyFunction);

    if (!_handle || !_backend || !_backend->process)
        return 0;

    if (in.empty() || out.empty())
        return 0;

    _mono.resize(std::max(in.size(), out.size()));
    _stereoIn.resize(in.size() * 2);
    _stereoOut.resize((static_cast<size_t>(std::ceil(in.size() * _ratio)) + OUTPUT_SLACK) * 2);

    convert_s16_to_float(_mono.data(), in.data(), in.size(), 1.0f);

    // The resamplers all work on stereo frames, so give them the same signal twice.
    for (size_t i = 0; i < in.size(); ++i) {
        _stereoIn[i * 2] = _mono[i];
        _stereoIn[i * 2 + 1] = _mono[i];
    }

    resampler_data data {
        .data_in = _stereoIn.data(),
        .data_out = _stereoOut.data(),
        .input_frames = in.size(),
        .output_frames = 0,
        .ratio = _ratio,
    };

    _backend->process(_handle, &data);

    // The resampler decides how many frames it produces,
    // which may be more than the caller has room for.
    size_t frames = std::min(data.output_frames, out.size());
    for (size_t i = 0; i < frames; ++i) {
        _mono[i] = _stereoOut[i * 2];
    }

    convert_float_to_s16(out.data(), _mono.data(), frames);

    return frames;
}
