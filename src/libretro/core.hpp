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

#ifndef MELONDSDS_CORE_HPP
#define MELONDSDS_CORE_HPP

#include <cstddef>
#include <memory>
#include <span>

#include "config.hpp"
#include "retro/info.hpp"
#include "screenlayout.hpp"
#include "PlatformOGLPrivate.h"

struct retro_game_info;
struct retro_system_av_info;

namespace melonDS {
    class NDS;
}

namespace MelonDsDs {
    namespace error {
        class ErrorScreen;
    }

    class CoreState {
    public:
        CoreState(bool init) noexcept;
        ~CoreState() noexcept;

        // TODO: Make private
        std::unique_ptr<melonDS::NDS> Console = nullptr;

        // TODO: Make private
        CoreConfig Config {};
        [[nodiscard]] bool IsInitialized() const noexcept { return initialized; }

        [[nodiscard]] retro_system_av_info GetSystemAvInfo() const noexcept;
        void Reset();
        void Run() noexcept;
        size_t SerializeSize() const noexcept;
        bool Serialize(std::span<std::byte> data) const noexcept;
        bool Unserialize(std::span<const std::byte> data) noexcept;
        void CheatSet(unsigned index, bool enabled, std::string_view code) noexcept;
        bool LoadGame(std::span<const retro_game_info> game);
        void UnloadGame() noexcept;
        std::span<std::byte> GetMemory(unsigned id) noexcept;
    private:
        ScreenLayoutData _screenLayout {};
        std::optional<retro::GameInfo> _ndsInfo = std::nullopt;
        std::optional<retro::GameInfo> _gbaInfo = std::nullopt;
        std::optional<retro::GameInfo> _gbaSaveInfo = std::nullopt;
        mutable std::optional<size_t> _savestateSize = std::nullopt;
        std::unique_ptr<error::ErrorScreen> _messageScreen = nullptr;
        bool initialized = false;
    };

    // TODO: Replace with a unique_ptr
    extern CoreState Core;
}
#endif //MELONDSDS_CORE_HPP
