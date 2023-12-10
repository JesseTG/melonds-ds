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

#include <array>
#include <cstddef>
#include <memory>
#include <regex>
#include <span>

#include "config.hpp"
#include "retro/info.hpp"
#include "screenlayout.hpp"
#include "PlatformOGLPrivate.h"
#include "sram.hpp"

struct retro_game_info;
struct retro_system_av_info;

namespace retro::task {
    class TaskSpec;
}

namespace melonDS {
    class NDS;

    namespace DSi_NAND {
        class NANDImage;
    }
}

namespace MelonDsDs {
    class RenderState;
    class config_exception;

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
        [[nodiscard]] bool IsInitialized() const noexcept { return _initialized; }

        [[nodiscard]] retro_system_av_info GetSystemAvInfo() const noexcept;
        [[gnu::hot]] void Run() noexcept;
        void Reset();
        size_t SerializeSize() const noexcept;
        [[gnu::hot]] bool Serialize(std::span<std::byte> data) const noexcept;
        bool Unserialize(std::span<const std::byte> data) noexcept;
        void CheatSet(unsigned index, bool enabled, std::string_view code) noexcept;
        bool LoadGame(unsigned type, std::span<const retro_game_info> game) noexcept;
        void UnloadGame() noexcept;
        std::byte* GetMemoryData(unsigned id) noexcept;
        size_t GetMemorySize(unsigned id) noexcept;
        void ResetRenderState();
        void DestroyRenderState();
    private:
        static constexpr auto REGEX_OPTIONS = std::regex_constants::ECMAScript | std::regex_constants::optimize;
        [[gnu::cold]] bool RunDeferredInitialization() noexcept;
        [[gnu::cold]] void RunFirstFrame() noexcept;
        [[gnu::cold]] void LoadGameDeferred();
        [[gnu::cold]] static void SetConsoleTime(melonDS::NDS& nds) noexcept;
        [[gnu::cold]] void SetUpDirectBoot(melonDS::NDS& nds, const retro::GameInfo& game) noexcept;
        [[gnu::cold]] void UninstallDsiware(melonDS::DSi_NAND::NANDImage& nand) noexcept;
        [[gnu::hot]] static void RenderAudio(melonDS::NDS& nds) noexcept;
        [[gnu::cold]] bool InitErrorScreen(const config_exception& e) noexcept;
        [[gnu::cold]] void InitContent(unsigned type, std::span<const retro_game_info> game);
        [[gnu::cold]] void InitRenderer();

        retro::task::TaskSpec PowerStatusUpdateTask() noexcept;
        retro::task::TaskSpec OnScreenDisplayTask() noexcept;
        retro::task::TaskSpec FlushGbaSramTask() noexcept;
        void FlushGbaSram(const retro::GameInfo& gbaSaveInfo) noexcept;
        retro::task::TaskSpec FlushFirmwareTask(string_view firmwareName) noexcept;
        void InitFlushFirmwareTask() noexcept;
        void FlushFirmware(string_view firmwarePath, string_view wfcSettingsPath) noexcept;
        void InitGbaSram(GbaCart& gbaCart, const retro::GameInfo& gbaSaveInfo);
        [[gnu::cold]] void InitNdsSave(const NdsCart &nds_cart);

        [[gnu::hot]] void ReadMicrophone(melonDS::NDS& nds, InputState& inputState) noexcept;
        ScreenLayoutData _screenLayout {};
        InputState _inputState {};
        std::optional<retro::GameInfo> _ndsInfo = std::nullopt;
        std::optional<retro::GameInfo> _gbaInfo = std::nullopt;
        std::optional<retro::GameInfo> _gbaSaveInfo = std::nullopt;
        std::optional<sram::SaveManager> _ndsSaveManager = std::nullopt;
        std::optional<sram::SaveManager> _gbaSaveManager = std::nullopt;
        std::optional<int> _timeToGbaFlush = std::nullopt;
        std::optional<int> _timeToFirmwareFlush = std::nullopt;
        mutable std::optional<size_t> _savestateSize = std::nullopt;
        std::unique_ptr<error::ErrorScreen> _messageScreen = nullptr;
        std::unique_ptr<RenderState> _renderState = nullptr;
        // TODO: Switch to compile time regular expressions (see https://compile-time.re)
        std::regex _cheatSyntax { "^\\s*[0-9A-Fa-f]{8}([+\\s]*[0-9A-Fa-f]{8})*$", REGEX_OPTIONS };
        std::regex _tokenSyntax { "[0-9A-Fa-f]{8}", REGEX_OPTIONS };
        bool _initialized = false;
        bool _firstFrameRun = false;
        bool _deferredInitializationPending = false;
        bool _micStateToggled = false;
        uint32_t _flushTaskId = 0;
    };

    extern CoreState Core;
}
#endif //MELONDSDS_CORE_HPP
