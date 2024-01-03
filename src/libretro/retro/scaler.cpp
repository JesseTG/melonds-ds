/*
    Copyright 2023 Jesse Talavera-Greenberg

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

#include "scaler.hpp"

#include <stdexcept>
#include <retro_assert.h>
#include <fmt/format.h>

#include "format.hpp"
#include "tracy.hpp"

constexpr unsigned PixelSize(scaler_pix_fmt fmt) noexcept {
    switch (fmt) {
        case SCALER_FMT_ARGB8888:
        case SCALER_FMT_ABGR8888:
        case SCALER_FMT_YUYV: // TODO: Should this be 2 instead?
            return 4;
        case SCALER_FMT_0RGB1555:
        case SCALER_FMT_RGB565:
        case SCALER_FMT_RGBA4444:
            return 2;
        case SCALER_FMT_BGR24:
            return 3;
        default:
            return 0;
    }
}

retro::Scaler::Scaler(
    scaler_pix_fmt in_fmt,
    scaler_pix_fmt out_fmt,
    scaler_type type,
    unsigned in_width,
    unsigned in_height,
    unsigned out_width,
    unsigned out_height
) {
    scaler.in_fmt = in_fmt;
    scaler.in_width = in_width;
    scaler.in_height = in_height;
    scaler.in_stride = in_width * PixelSize(in_fmt);

    scaler.out_fmt = out_fmt;
    scaler.out_width = out_width;
    scaler.out_height = out_height;
    scaler.out_stride = out_width * PixelSize(out_fmt);

    scaler.scaler_type = type;

    bool ok;
    {
        ZoneScopedN("scaler_ctx_gen_filter");
        ok = scaler_ctx_gen_filter(&scaler);
    }
    if (!ok) {
        std::string error = fmt::format(
            "Failed to generate scaler filter from {}x{} {} to {}x{} {}",
            in_width,
            in_height,
            in_fmt,
            out_width,
            out_height,
            out_fmt
        );

        throw std::runtime_error(error);
    }
}

retro::Scaler::Scaler(scaler_pix_fmt in_fmt, scaler_pix_fmt out_fmt, unsigned width, unsigned height) :
    Scaler(in_fmt, out_fmt, SCALER_TYPE_POINT, width, height, width, height) {
}

retro::Scaler::~Scaler() noexcept {
    ZoneScopedN("scaler_ctx_gen_reset");
    scaler_ctx_gen_reset(&scaler);
}

retro::Scaler::Scaler(Scaler&& other) noexcept {
    ZoneScopedN("scaler_ctx_gen_reset");
    scaler_ctx_gen_reset(&scaler);
    scaler = other.scaler;
    other.scaler = {};
}

retro::Scaler& retro::Scaler::operator=(Scaler&& other) noexcept {
    if (this != &other) {
        ZoneScopedN("scaler_ctx_gen_reset");
        scaler_ctx_gen_reset(&scaler);
        scaler = other.scaler;
        other.scaler = {};
    }
    return *this;
}

void retro::Scaler::SetOutSize(unsigned width, unsigned height) noexcept {
    if (scaler.out_width == width && scaler.out_height == height)
        return;

    scaler.out_width = width;
    scaler.out_height = height;
    scaler.out_stride = width * PixelSize(scaler.out_fmt);
    ZoneScopedN("scaler_ctx_gen_filter");
    scaler_ctx_gen_filter(&scaler);
}

void retro::Scaler::Scale(void *output, const void *input) noexcept {
    if (output == nullptr || input == nullptr)
        return;

    ZoneScopedN("scaler_ctx_scale");
    scaler_ctx_scale(&scaler, output, input);
}