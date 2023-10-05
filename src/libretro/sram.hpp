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
#include "memory.hpp"

//! Definitions for managing SRAM.

namespace retro::task {
    class TaskSpec;
}

struct retro_game_info;

namespace melonds::sram {
    void init();
    void reset() noexcept;
    void deinit() noexcept;
    void InitNdsSave(const NdsCart &nds_cart);
    void InitGbaSram(GbaCart& gba_cart, const struct retro_game_info& gba_save_info);

    retro::task::TaskSpec FlushGbaSramTask(const retro_game_info& gba_save_info) noexcept;
    retro::task::TaskSpec FlushFirmwareTask(std::string_view firmwareName) noexcept;

    /// An intermediate save buffer used as a staging ground between retro_get_memory and NDSCart::LoadSave.
    class SaveManager {
    public:
        SaveManager(u32 initialLength);

        ~SaveManager();

        SaveManager(const SaveManager &) = delete;

        SaveManager(SaveManager &&) = delete;

        SaveManager &operator=(const SaveManager &) = delete;

        /// Signals that SRAM was recently updated.
        ///
        /// \param savedata Pointer to the entire SRAM buffer.
        /// Never changes during a game's lifetime.
        /// \param savelen Length of the entire SRAM buffer.
        /// Never changes during a game's lifetime.
        /// \param writeoffset Starting position of the updated data
        /// \param writelen Length of the updated data.
        void Flush(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen);

        [[nodiscard]] const u8 *Sram() const {
            return _sram;
        }

        u8 *Sram() {
            return _sram;
        }

        [[nodiscard]] u32 SramLength() const {
            return _sram_length;
        }

    private:
        u8 *_sram;
        u32 _sram_length;
    };

    extern std::unique_ptr<SaveManager> NdsSaveManager;

    extern std::unique_ptr<SaveManager> GbaSaveManager;
}

#endif //MELONDS_DS_SRAM_HPP
