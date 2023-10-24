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

#ifndef MELONDS_DS_SCALER_HPP
#define MELONDS_DS_SCALER_HPP

#include <gfx/scaler/scaler.h>

namespace retro {
    class Scaler {
    public:
        Scaler() noexcept;
        Scaler(scaler_pix_fmt in_fmt, scaler_pix_fmt out_fmt, scaler_type type, unsigned in_width, unsigned in_height, unsigned out_width, unsigned out_height);
        Scaler(scaler_pix_fmt in_fmt, scaler_pix_fmt out_fmt, unsigned width, unsigned height);
        Scaler(scaler_ctx&& ctx) noexcept;
        ~Scaler() noexcept;
        Scaler(const Scaler&) = delete;
        Scaler(Scaler&&) noexcept;
        Scaler& operator=(const Scaler&) = delete;
        Scaler& operator=(Scaler&&) noexcept;

        void Scale(void *output, const void *input) noexcept;
        unsigned InWidth() const noexcept { return scaler.in_width; }
        unsigned InHeight() const noexcept { return scaler.in_height; }
        unsigned InStride() const noexcept { return scaler.in_stride; }
        unsigned OutWidth() const noexcept { return scaler.out_width; }
        unsigned OutHeight() const noexcept { return scaler.out_height; }
        unsigned OutStride() const noexcept { return scaler.out_stride; }
    private:
        scaler_ctx scaler;
    };
}

#endif //MELONDS_DS_SCALER_HPP
