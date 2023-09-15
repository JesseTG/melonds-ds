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

#ifndef MELONDS_DS_MICROPHONE_HPP
#define MELONDS_DS_MICROPHONE_HPP

#include <cstdint>
#include <optional>

namespace retro::microphone {
    [[deprecated("Wrap this in a class instead")]] void init_interface() noexcept;
    [[deprecated("Wrap this in a class instead")]] void clear_interface() noexcept;

    [[deprecated("Wrap this in a class instead")]] bool is_interface_available() noexcept;
    [[deprecated("Wrap this in a class instead")]] bool set_open(bool open) noexcept;
    [[deprecated("Wrap this in a class instead")]] bool is_open() noexcept;
    [[deprecated("Wrap this in a class instead")]] bool set_state(bool on) noexcept;
    [[deprecated("Wrap this in a class instead")]] std::optional<bool> get_state() noexcept;
    [[deprecated("Wrap this in a class instead")]] std::optional<int> read(int16_t* samples, size_t num_samples) noexcept;
}

#endif //MELONDS_DS_MICROPHONE_HPP
