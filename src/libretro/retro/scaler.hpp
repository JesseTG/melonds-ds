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
        Scaler(scaler_pix_fmt in_fmt, scaler_pix_fmt out_fmt, scaler_type type, unsigned in_width, unsigned in_height, unsigned out_width, unsigned out_height);
        Scaler(scaler_pix_fmt in_fmt, scaler_pix_fmt out_fmt, unsigned width, unsigned height);
        ~Scaler() noexcept;
        Scaler(const Scaler&) = delete;
        Scaler(Scaler&&) noexcept;
        Scaler& operator=(const Scaler&) = delete;
        Scaler& operator=(Scaler&&) noexcept;

        [[nodiscard]] scaler_type GetScalerType() const noexcept { return scaler.scaler_type; }
        void SetScalerType(scaler_type type) noexcept {
            if (scaler.scaler_type != type) {
                _dirty = true;
            }
            scaler.scaler_type = type;
        }
        void Scale(void *output, const void *input) noexcept;
        [[nodiscard]] unsigned InWidth() const noexcept { return scaler.in_width; }
        [[nodiscard]] unsigned InHeight() const noexcept { return scaler.in_height; }
        [[nodiscard]] unsigned InStride() const noexcept { return scaler.in_stride; }

        void SetOutSize(unsigned width, unsigned height) noexcept;
        [[nodiscard]] unsigned OutWidth() const noexcept { return scaler.out_width; }
        [[nodiscard]] unsigned OutHeight() const noexcept { return scaler.out_height; }
        [[nodiscard]] unsigned OutStride() const noexcept { return scaler.out_stride; }
    private:
        void Update() noexcept;

        scaler_ctx scaler {};
        bool _dirty = false;
    };
}

#endif //MELONDS_DS_SCALER_HPP
