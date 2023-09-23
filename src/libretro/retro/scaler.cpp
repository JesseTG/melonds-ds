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

const char* PixelFormatName(scaler_pix_fmt fmt) noexcept {
    switch (fmt) {
        case SCALER_FMT_ARGB8888:
            return "ARGB8888";
        case SCALER_FMT_ABGR8888:
            return "ABGR8888";
        case SCALER_FMT_0RGB1555:
            return "0RGB1555";
        case SCALER_FMT_RGB565:
            return "RGB565";
        case SCALER_FMT_RGBA4444:
            return "RGBA4444";
        case SCALER_FMT_BGR24:
            return "BGR24";
        case SCALER_FMT_YUYV:
            return "YUYV";
        default:
            return "<unknown>";
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

    if (!scaler_ctx_gen_filter(&scaler)) {
        char error[256];
        snprintf(
            error,
            sizeof(error),
            "Failed to generate scaler filter from %ux%u %s to %ux%u %s",
            in_width,
            in_height,
            PixelFormatName(in_fmt),
            out_width,
            out_height,
            PixelFormatName(out_fmt)
        );
        throw std::runtime_error(error);
    }
}

retro::Scaler::~Scaler() noexcept {
    scaler_ctx_gen_reset(&scaler);
}

retro::Scaler::Scaler(Scaler&& other) noexcept {
    scaler_ctx_gen_reset(&scaler);
    scaler = other.scaler;
    other.scaler = {};
}

retro::Scaler& retro::Scaler::operator=(Scaler&& other) noexcept {
    if (this != &other) {
        scaler_ctx_gen_reset(&scaler);
        scaler = other.scaler;
        other.scaler = {};
    }
    return *this;
}


void retro::Scaler::Scale(void *output, const void *input) noexcept {
    if (output == nullptr || input == nullptr)
        return;

    scaler_ctx_scale(&scaler, output, input);
}