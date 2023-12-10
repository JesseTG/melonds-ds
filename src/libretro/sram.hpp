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

#ifndef MELONDS_DS_SRAM_HPP
#define MELONDS_DS_SRAM_HPP

#include <cstdint>
#include <vector>

#include "libretro.hpp"

//! Definitions for managing SRAM.

namespace retro::task {
    class TaskSpec;
}

struct retro_game_info;

namespace MelonDsDs::sram  {
    /// An intermediate save buffer used as a staging ground between retro_get_memory and NDSCart::LoadSave.
    class SaveManager {
    public:
        explicit SaveManager(uint32_t initialLength);
        ~SaveManager();
        SaveManager(const SaveManager&) = delete;
        SaveManager(SaveManager&&) noexcept;
        SaveManager& operator=(const SaveManager &) = delete;
        SaveManager& operator=(SaveManager&&) noexcept;

        /// Signals that SRAM was recently updated.
        ///
        /// \param savedata Pointer to the entire SRAM buffer.
        /// Never changes during a game's lifetime.
        /// \param savelen Length of the entire SRAM buffer.
        /// Never changes during a game's lifetime.
        /// \param writeoffset Starting position of the updated data
        /// \param writelen Length of the updated data.
        void Flush(const uint8_t *savedata, uint32_t savelen, uint32_t writeoffset, uint32_t writelen);

        [[nodiscard]] const uint8_t *Sram() const { return _sram; }
        uint8_t *Sram() { return _sram; }
        [[nodiscard]] uint32_t SramLength() const { return _sram_length; }

    private:
        uint8_t *_sram;
        uint32_t _sram_length;
    };
}

#endif //MELONDS_DS_SRAM_HPP
