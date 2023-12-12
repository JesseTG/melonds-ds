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

#ifndef RETRO_MICROPHONE_HPP
#define RETRO_MICROPHONE_HPP

#include <optional>
#include <span>

#include <libretro.h>

namespace retro {
    class Microphone {
    public:
        ~Microphone() noexcept;
        Microphone(const Microphone&) = delete;
        Microphone& operator=(const Microphone&) = delete;
        Microphone(Microphone&&) noexcept;
        Microphone& operator=(Microphone&&) noexcept;
        std::optional<retro_microphone_params_t> GetParams() const noexcept;
        bool SetActive(bool on) noexcept;
        bool IsActive() const noexcept;
        int Read(std::span<int16_t> buffer) noexcept;

        static std::optional<Microphone> Open(const retro_microphone_interface& micInterface, retro_microphone_params_t params) noexcept;
    private:
        Microphone(retro_microphone_t* microphone, const retro_microphone_interface& micInterface) noexcept;
        retro_microphone_interface _microphoneInterface;
        retro_microphone_t* _microphone;
    };
}

#endif // RETRO_MICROPHONE_HPP
