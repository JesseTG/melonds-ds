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
#include <libretro.h>
#include <memory>
#include <regex>

#include <NDS.h>

#include "../config/config.hpp"
#include "../config/visibility.hpp"
#include "../message/error.hpp"
#include "../microphone.hpp"
#include "../render/render.hpp"
#include "../retro/info.hpp"
#include "../screenlayout.hpp"
#include "../PlatformOGLPrivate.h"
#include "../sram.hpp"
#include "net/net.hpp"
#include "net/mp.hpp"
#include "std/span.hpp"

struct retro_game_info;
struct retro_system_av_info;

namespace retro::task {
    class TaskSpec;
}


namespace melonDS {
    class NDS;
    struct AdapterData;

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
        CoreState() noexcept = default;
        ~CoreState() noexcept;
        CoreState(const CoreState&) = delete;
        CoreState& operator=(const CoreState&) = delete;
        CoreState(CoreState&&) noexcept = delete;
        CoreState& operator=(CoreState&&) noexcept = delete;

        [[nodiscard]] bool IsInitialized() const noexcept { return _initialized; }

        [[nodiscard]] retro_system_av_info GetSystemAvInfo() const noexcept;
        [[nodiscard]] retro_system_av_info GetSystemAvInfo(RenderMode renderer) const noexcept;
        [[gnu::hot]] void Run() noexcept;
        void Reset();
        size_t SerializeSize() const noexcept;
        [[gnu::hot]] bool Serialize(std::span<std::byte> data) const noexcept;
        bool Unserialize(std::span<const std::byte> data) noexcept;
        void CheatReset() noexcept;
        void CheatSet(unsigned index, bool enabled, std::string_view code) noexcept;
        bool LoadGame(unsigned type, std::span<const retro_game_info> game) noexcept;
        void UnloadGame() noexcept;
        std::byte* GetMemoryData(unsigned id) noexcept;
        size_t GetMemorySize(unsigned id) const noexcept;
        void ResetRenderState();
        void DestroyRenderState();
        int LanSendPacket(std::span<std::byte> data) noexcept;
        int LanRecvPacket(uint8_t* data) noexcept;

        void MpStarted(retro_netpacket_send_t send, retro_netpacket_poll_receive_t poll_receive) noexcept;
        void MpPacketReceived(const void *buf, size_t len, uint16_t client_id) noexcept;
        void MpStopped() noexcept;
        bool MpSendPacket(const Packet &p) noexcept;
        std::optional<Packet> MpNextPacket() noexcept;
        std::optional<Packet> MpNextPacketBlock() noexcept;
        bool MpActive() const noexcept;

        void WriteNdsSave(std::span<const std::byte> savedata, uint32_t writeoffset, uint32_t writelen) noexcept;
        void WriteGbaSave(std::span<const std::byte> savedata, uint32_t writeoffset, uint32_t writelen) noexcept;
        void WriteFirmware(const melonDS::Firmware& firmware, uint32_t writeoffset, uint32_t writelen) noexcept;
        bool UpdateOptionVisibility() noexcept;

        const melonDS::NDS* GetConsole() const noexcept { return Console.get(); }
        [[nodiscard]] const InputState& GetInputState() const noexcept { return _inputState; }
        [[nodiscard]] InputState& GetInputState() noexcept { return _inputState; }
        std::optional<RenderMode> GetRenderMode() const noexcept { return _renderState.GetRenderMode(); }
        const ScreenLayoutData& GetScreenLayoutData() const noexcept { return _screenLayout; }
    private:
        static constexpr auto REGEX_OPTIONS = std::regex_constants::ECMAScript | std::regex_constants::optimize;
        [[gnu::cold]] void ApplyConfig(const CoreConfig& config) noexcept;
        [[gnu::cold]] bool RunDeferredInitialization() noexcept;
        [[gnu::cold]] void InstallNdsSram() noexcept;
        [[gnu::cold]] void StartConsole();
        [[gnu::cold]] void SetConsoleTime(melonDS::NDS& nds) noexcept;
        [[gnu::cold]] void SetConsoleTime(melonDS::NDS& nds, local_seconds time) noexcept;
        [[gnu::cold]] void SetUpDirectBoot(melonDS::NDS& nds, const retro::GameInfo& game) noexcept;
        [[gnu::cold]] void UninstallDsiware(melonDS::DSi_NAND::NANDImage& nand) noexcept;
        [[gnu::cold]] static void ExportDsiwareSaveData(
            melonDS::DSi_NAND::NANDMount& nand,
            const retro::GameInfo& nds_info,
            const melonDS::NDSHeader& header,
            int type
        ) noexcept;
        [[gnu::hot]] static void RenderAudio(melonDS::NDS& nds) noexcept;
        [[gnu::cold]] bool InitErrorScreen(const config_exception& e) noexcept;
        [[gnu::cold]] void RenderErrorScreen() noexcept;
        [[gnu::cold]] void InitContent(unsigned type, std::span<const retro_game_info> game);

        const melonDS::AdapterData* SelectNetworkInterface(std::span<const melonDS::AdapterData> adapters) const noexcept;

        retro::task::TaskSpec PowerStatusUpdateTask() noexcept;
        retro::task::TaskSpec OnScreenDisplayTask() noexcept;
        retro::task::TaskSpec FlushGbaSramTask() noexcept;
        void FlushGbaSram(const retro::GameInfo& gbaSaveInfo) noexcept;
        retro::task::TaskSpec FlushFirmwareTask(string_view firmwareName) noexcept;
        void InitFlushFirmwareTask() noexcept;
        void FlushFirmware(string_view firmwarePath, string_view wfcSettingsPath) noexcept;
        [[gnu::cold]] void InitNdsSave(const NdsCart &nds_cart);

        std::unique_ptr<melonDS::NDS> Console = nullptr;
        NetState _netState;
        CoreConfig Config {};
        CoreOptionVisibility _optionVisibility {};
        ScreenLayoutData _screenLayout {};
        InputState _inputState {};
        MicrophoneState _micState {};
        RenderStateWrapper _renderState {};
        MpState _mpState {};
        std::optional<retro::GameInfo> _ndsInfo = std::nullopt;
        std::optional<retro::GameInfo> _gbaInfo = std::nullopt;
        std::optional<retro::GameInfo> _gbaSaveInfo = std::nullopt;
        std::optional<sram::SaveManager> _ndsSaveManager = std::nullopt;
        std::optional<sram::SaveManager> _gbaSaveManager = std::nullopt;
        std::optional<int> _timeToGbaFlush = std::nullopt;
        std::optional<int> _timeToFirmwareFlush = std::nullopt;
        mutable std::optional<size_t> _savestateSize = std::nullopt;
        bool _syncClock = false;
        std::unique_ptr<error::ErrorScreen> _messageScreen = nullptr;
        // TODO: Switch to compile time regular expressions (see https://compile-time.re)
        std::regex _cheatSyntax { "^\\s*[0-9A-Fa-f]{8}([+\\s-]*[0-9A-Fa-f]{8})*$", REGEX_OPTIONS };
        std::regex _tokenSyntax { "[0-9A-Fa-f]{8}", REGEX_OPTIONS };
        // This object is meant to be stored in a placement-new'd byte array,
        // so having this flag lets us detect if the core has been initialized
        // regardless of the state of the underlying resources
        const bool _initialized = true;
        bool _ndsSramInstalled = false;
        bool _deferredInitializationPending = false;
        uint32_t _flushTaskId = 0;
    };
}
#endif //MELONDSDS_CORE_HPP
