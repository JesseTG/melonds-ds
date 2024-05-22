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

#include "core.hpp"

#include <charconv>
#include <DSi.h>
#include <GPU3D_OpenGL.h>
#include <GPU3D_Soft.h>

#include <libretro.h>
#include <retro_assert.h>

#include <NDS.h>
#include <compat/strl.h>
#include <file/file_path.h>

#include "../config/console.hpp"
#include "../exceptions.hpp"
#include "../format.hpp"
#include "../info.hpp"
#include "../microphone.hpp"
#include "../message/error.hpp"
#include "../render/render.hpp"
#include "../retro/task_queue.hpp"
#include "render/software.hpp"

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include "../render/opengl.hpp"
#endif

using std::span;
using namespace melonDS::DSi_NAND;

constexpr size_t DS_MEMORY_SIZE = 0x400000;
constexpr size_t DSI_MEMORY_SIZE = 0x1000000;
static const char* const INTERNAL_ERROR_MESSAGE =
    "An internal error occurred with melonDS DS. "
    "Please contact the developer with the log file.";

static const char* const UNKNOWN_ERROR_MESSAGE =
    "An unknown error has occurred with melonDS DS. "
    "Please contact the developer with the log file.";

date::local_seconds LocalTime() noexcept {
    using namespace std::chrono;

    std::tm tm = fmt::localtime(system_clock::to_time_t(system_clock::now()));

    year_month_day date {year{tm.tm_year + 1900}, month{tm.tm_mon + 1u}, day{(unsigned)tm.tm_mday}};
    seconds time = hours{tm.tm_hour} + minutes{tm.tm_min} + seconds{tm.tm_sec};

    return static_cast<local_days>(date) + time;
}

MelonDsDs::CoreState::~CoreState() noexcept {
    ZoneScopedN(TracyFunction);
    Console = nullptr;
    melonDS::NDS::Current = nullptr;
}

retro_system_av_info MelonDsDs::CoreState::GetSystemAvInfo(RenderMode renderer) const noexcept {
    return {
        .geometry = _screenLayout.Geometry(renderer),
        .timing {
            .fps = 32.0f * 1024.0f * 1024.0f / 560190.0f,
            .sample_rate = 32.0f * 1024.0f,
        },
    };
}

retro_system_av_info MelonDsDs::CoreState::GetSystemAvInfo() const noexcept {
#ifndef NDEBUG
    if (!_messageScreen) {
        retro_assert(Console != nullptr);
    }
#endif

    std::optional<RenderMode> renderer = _renderState.GetRenderMode();
    retro_assert(renderer.has_value());

    return GetSystemAvInfo(*renderer);
}

void MelonDsDs::CoreState::UnloadGame() noexcept {
    if (Console && Console->IsRunning()) {
        // If the NDS wasn't already stopped due to some internal event...
        Console->Stop();
    }

    if (_ndsInfo) {
        // If this session involved a loaded DS game...

        retro_assert(!_ndsInfo->GetData().empty());
        const melonDS::NDSHeader& header = *reinterpret_cast<const melonDS::NDSHeader*>(_ndsInfo->GetData().data());
        if (header.IsDSiWare()) {
            // And that game was a DSiWare game...
            retro_assert(Console->ConsoleType == 1);
            retro_assert(dynamic_cast<melonDS::DSi*>(Console.get()) != nullptr);

            melonDS::DSi& dsi = *static_cast<melonDS::DSi*>(Console.get());
            UninstallDsiware(dsi.GetNAND());
        }
    }

    Console = nullptr;
    melonDS::NDS::Current = nullptr;
}

void MelonDsDs::CoreState::Run() noexcept {
    ZoneScopedN(TracyFunction);

    if (_deferredInitializationPending && !RunDeferredInitialization()) [[unlikely]] {
        // If we needed to run any extra setup, but that process failed...
        retro::shutdown();
        return;
    }

    if (_messageScreen) [[unlikely]] {
        RenderErrorScreen();
        return;
    }

    retro_assert(Console != nullptr);
    melonDS::NDS& nds = *Console;

    if (retro::is_variable_updated()) [[unlikely]] {
        // If any settings have changed...
        ParseConfig(Config);
        ApplyConfig(Config);
        UpdateConsole(Config, nds);
    }

    if (!_ndsSramInstalled) [[unlikely]] {
        InstallNdsSram();
        _ndsSramInstalled = true;
    }

    if (_renderState.Ready()) [[likely]] {
        // If the global state needed for rendering is ready...
        HandleInput(nds, _inputState, _screenLayout);
        _micState.SetMicButtonState(_inputState.MicButtonDown());
        std::array<int16_t, 735> buffer {};
        _micState.Read(buffer);
        nds.MicInputFrame(buffer.data(), buffer.size());

        if (_screenLayout.Dirty()) {
            // If the active screen layout has changed (either by settings or by hotkey)...

            // Apply the new screen layout
            _screenLayout.Update();

            RenderMode renderer = Console->GPU.GetRenderer3D().Accelerated ? RenderMode::OpenGl : RenderMode::Software;
            // And update the geometry
            if (!retro::set_geometry(_screenLayout.Geometry(renderer))) {
                retro::warn("Failed to update geometry after screen layout change");
            }

            _renderState.RequestRefresh();
        }

        if (_syncClock) {
            SetConsoleTime(nds, LocalTime());
        }

        // NDS::RunFrame renders the Nintendo DS state to a framebuffer,
        // which is then drawn to the screen by _renderState.Render
        {
            ZoneScopedN("NDS::RunFrame");
            nds.RunFrame();
        }

        _renderState.Render(nds, _inputState, Config, _screenLayout);
        RenderAudio(*Console);

        retro::task::check();
    }
}

void MelonDsDs::CoreState::Reset() {
    ZoneScopedN(TracyFunction);

    if (_messageScreen) {
        retro::set_error_message("Please follow the advice on this screen, then unload/reload the core.");
        return;
        // TODO: Allow the game to be reset from the error screen
        // (gotta reinitialize the DS here)
    }

    // Flush all data before resetting
    _timeToFirmwareFlush = 0;
    _timeToGbaFlush = 0;
    retro::task::find([this](retro::task::TaskHandle& task) {
        if (task.Identifier() == _flushTaskId) {
            // If this is the flush task we want to cancel...
            task.Cancel();
            return true;
        }
        return false; // Keep looking...
    });
    retro::task::check();
    _savestateSize = std::nullopt;

    retro_assert(Console != nullptr);
    RegisterCoreOptions();
    ParseConfig(Config);
    ApplyConfig(Config);
    _syncClock = Config.StartTimeMode() == StartTimeMode::Sync;

    std::vector<uint8_t> ndsSram(Console->GetNDSSaveLength());
    if (Console->GetNDSSaveLength() && Console->GetNDSSave()) {
        memcpy(ndsSram.data(), Console->GetNDSSave(), Console->GetNDSSaveLength());
    }

    std::vector<uint8_t> gbaSram(Console->GetGBASaveLength());
    if (Console->GetGBASaveLength() && Console->GetGBASave()) {
        memcpy(gbaSram.data(), Console->GetGBASave(), Console->GetGBASaveLength());
    }

    Console = nullptr;
    melonDS::NDS::Current = nullptr;
    Console = CreateConsole(
        Config,
        _ndsInfo ? &*_ndsInfo : nullptr,
        _gbaInfo ? &*_gbaInfo : nullptr,
        _gbaSaveInfo ? &*_gbaSaveInfo : nullptr
    );
    retro_assert(Console != nullptr);
    melonDS::NDS::Current = Console.get();
    // TODO: Don't throw out the NDS object (unless changing console type), customize it instead
    if (!ndsSram.empty()) {
        Console->SetNDSSave(ndsSram.data(), ndsSram.size());
    }

    if (!gbaSram.empty()) {
        Console->SetGBASave(gbaSram.data(), gbaSram.size());
    }

    _ndsSramInstalled = false;
    InitFlushFirmwareTask();

    StartConsole();
}


void MelonDsDs::CoreState::RenderAudio(melonDS::NDS& nds) noexcept {
    ZoneScopedN(TracyFunction);
    int16_t audio_buffer[0x1000]; // 4096 samples == 2048 stereo frames
    uint32_t size = std::min(nds.SPU.GetOutputSize(), static_cast<int>(sizeof(audio_buffer) / (2 * sizeof(int16_t))));
    // Ensure that we don't overrun the buffer

    size_t read = nds.SPU.ReadOutput(audio_buffer, size);
    retro::audio_sample_batch(audio_buffer, read);
}

bool MelonDsDs::CoreState::RunDeferredInitialization() noexcept {
    ZoneScopedN(TracyFunction);
    retro_assert(Console != nullptr);
    try {
        retro::debug("Starting deferred initialization");
        StartConsole();
        _deferredInitializationPending = false;

        retro::debug("Completed deferred initialization");
    }
    catch (const config_exception& e) {
        retro::error("Deferred initialization failed; displaying error screen");
        retro::error("{}", e.what());
        retro::set_error_message(e.user_message());
        if (!InitErrorScreen(e))
            return false;
    }
    catch (const emulator_exception& e) {
        retro::error("Deferred initialization failed; exiting core");
        retro::error("{}", e.what());
        retro::set_error_message(e.user_message());
        return false;
    }
    catch (const std::exception& e) {
        retro::error("Deferred initialization failed; exiting core");
        retro::set_error_message(e.what());
        return false;
    }
    catch (...) {
        retro::error("Deferred initialization failed; exiting core");
        retro::set_error_message(UNKNOWN_ERROR_MESSAGE);
        return false;
    }

    return true;
}

bool MelonDsDs::CoreState::InitErrorScreen(const config_exception& e) noexcept {
    ZoneScopedN(TracyFunction);
    retro_assert(_messageScreen == nullptr);
    if (getenv("MELONDSDS_SKIP_ERROR_SCREEN")) {
        // This part is for the test suite
        retro::error("Skipping error screen due to the environment variable MELONDSDS_SKIP_ERROR_SCREEN");
        return false;
    }

    retro::task::reset();
    _messageScreen = std::make_unique<error::ErrorScreen>(e, language);
    Config.SetConfiguredRenderer(RenderMode::Software);
    _screenLayout.Update();
    retro::error("Error screen initialized");
    return true;
}

void MelonDsDs::CoreState::RenderErrorScreen() noexcept {
    assert(_messageScreen != nullptr);

    _screenLayout.Update();
    _renderState.Render(*_messageScreen, Config, _screenLayout);
}

std::chrono::system_clock::time_point ToSystemTime(std::chrono::local_seconds time) noexcept {
    return std::chrono::system_clock::from_time_t(time.time_since_epoch().count());
}

void MelonDsDs::CoreState::InstallNdsSram() noexcept {
    ZoneScopedN(TracyFunction);

    if (_ndsSramInstalled) return;

    // Apply the save data from the core's SRAM buffer to the cart's SRAM;
    // we need to do this in the first frame of retro_run because
    // retro_get_memory_data is used to copy the loaded SRAM
    // in between retro_load and the first retro_run call.

    // Nintendo DS SRAM is loaded by the frontend
    // and copied into NdsSaveManager via the pointer returned by retro_get_memory.
    // This is where we install the SRAM data into the emulated DS.
    if (_ndsInfo && _ndsSaveManager && _ndsSaveManager->SramLength() > 0) {
        // If we're loading a NDS game that has SRAM...
        ZoneScopedN("NDS::SetNDSSave");
        Console->SetNDSSave(_ndsSaveManager->Sram(), _ndsSaveManager->SramLength());
        retro::debug("Installed {}-byte SRAM", _ndsSaveManager->SramLength());
    }

    _ndsSramInstalled = true;
}

void MelonDsDs::CoreState::SetConsoleTime(melonDS::NDS& nds) noexcept {
    ZoneScopedN(TracyFunction);

    local_seconds now = LocalTime();
    local_seconds targetTime;

    switch (Config.StartTimeMode()) {
        case StartTimeMode::Sync:
        case StartTimeMode::Real: {
            targetTime = now;
            retro::debug("Starting the RTC at {:%F %r} (local time)", ToSystemTime(targetTime));
            break;
        }
        case StartTimeMode::Relative: {
            minutes offset = Config.RelativeDateTimeOffset();
            targetTime = now + offset;
            retro::debug("Starting the RTC at {:%F %r} ({}y, {}, {}, {} from now)",
                ToSystemTime(targetTime),
                Config.RelativeYearOffset().count(),
                Config.RelativeDayOffset(),
                Config.RelativeHourOffset(),
                Config.RelativeMinuteOffset()
            );
            break;
        }
        case StartTimeMode::Absolute: {
            const auto tpm = floor<seconds>(now);
            const auto dp = floor<days>(tpm);
            auto time = make_time(tpm-dp);
            targetTime = Config.AbsoluteStartDateTime() + time.seconds();
            retro::debug("Starting the RTC at {:%F %r} (ignoring the local time)", ToSystemTime(targetTime));
            break;
        }
    }

    SetConsoleTime(nds, targetTime);
}

void MelonDsDs::CoreState::SetConsoleTime(melonDS::NDS& nds, local_seconds time) noexcept {
    auto today = year_month_day{floor<days>(time)};
    const auto tpm = floor<seconds>(time);
    const auto dp = floor<days>(tpm);
    auto now = make_time(tpm-dp);
    nds.RTC.SetDateTime(
        static_cast<int>(today.year()),
        static_cast<unsigned>(today.month()),
        static_cast<unsigned>(today.day()),
        now.hours().count(),
        now.minutes().count(),
        now.seconds().count()
    );
}

// When requesting an OpenGL context, we may not get it immediately.
// So we need to wait until we do.
void MelonDsDs::CoreState::StartConsole() {
    ZoneScopedN(TracyFunction);

    retro_assert(Console != nullptr); // This function should only be called if the console is initialized

    _renderState.UpdateRenderer(Config, *Console);

    {
        ZoneScopedN("NDS::Reset");
        Console->Reset();
    }

    SetConsoleTime(*Console);

    if (_ndsInfo && Console->GetNDSCart() && !Console->GetNDSCart()->GetHeader().IsDSiWare()) {
        SetUpDirectBoot(*Console, *_ndsInfo);
    }

    Console->Start();

    retro::info("Started emulated console");
}

// Decrypts the ROM's secure area
void MelonDsDs::CoreState::SetUpDirectBoot(melonDS::NDS& nds, const retro::GameInfo& game) noexcept {
    ZoneScopedN(TracyFunction);
    if (Config.BootMode() == BootMode::Direct || nds.NeedsDirectBoot()) {
        char game_name[256];

        if (const char* ptr = path_basename(game.GetPath().data()); ptr)
            strlcpy(game_name, ptr, sizeof(game_name));
        else
            strlcpy(game_name, game.GetPath().data(), sizeof(game_name));

        {
            ZoneScopedN("NDS::SetupDirectBoot");
            nds.SetupDirectBoot(game_name);
        }
        retro::debug("Initialized direct boot for \"{}\"", game_name);
    }
}

void MelonDsDs::CoreState::InitFlushFirmwareTask() noexcept
{
    retro_assert(Console != nullptr);
    string_view firmwareName = Config.FirmwarePath(static_cast<ConsoleType>(Console->ConsoleType));
    if (retro::task::TaskSpec flushTask = FlushFirmwareTask(firmwareName)) {
        _flushTaskId = flushTask.Identifier();
        retro::task::push(std::move(flushTask));
    }
    else {
        retro::set_error_message("System path not found, changes to firmware settings won't be saved.");
    }
}

void MelonDsDs::CoreState::ResetRenderState() {
    _renderState.ContextReset(*Console, Config);
}

void MelonDsDs::CoreState::DestroyRenderState() {
    _renderState.ContextDestroyed();
}

bool MelonDsDs::CoreState::LoadGame(unsigned type, std::span<const retro_game_info> game) noexcept try {
    ZoneScopedN(TracyFunction);

    printf("Initialize languageCoreState::LoadGame  ");

    // Initialize the language.
    unsigned lang = 0;
    if (retro::environment(RETRO_ENVIRONMENT_GET_LANGUAGE, (void*)&lang)) {
        language = (enum retro_language)lang;
    }

    InitContent(type, game);

    // ...then load the game.
    if (!retro::set_pixel_format(RETRO_PIXEL_FORMAT_XRGB8888)) {
        throw environment_exception(
            "Failed to set the required XRGB8888 pixel format for rendering; it may not be supported.");
    }

    if (RegisterCoreOptions()) {
        ParseConfig(Config);
        _optionVisibility.Update();
    }
    ApplyConfig(Config);
    // Must initialize the render state if using OpenGL (so the function pointers can be loaded

    _syncClock = Config.StartTimeMode() == StartTimeMode::Sync;
    retro_assert(Console == nullptr);
    // Instantiates the console with games and save data installed
    Console = CreateConsole(
        Config,
        _ndsInfo ? &*_ndsInfo : nullptr,
        _gbaInfo ? &*_gbaInfo : nullptr,
        _gbaSaveInfo ? &*_gbaSaveInfo : nullptr
    );

    retro_assert(Console != nullptr);
    melonDS::NDS::Current = Console.get();


    if (Console->GetNDSCart()) {
        assert(!Console->GetNDSCart()->GetHeader().IsDSiWare());
        // DSi mode should've been forced if loading a DSiWare game
        InitNdsSave(*Console->GetNDSCart());
    }

    if (_gbaInfo && _gbaSaveInfo && Console->GetGBASave() && Console->GetGBASaveLength()) {
        // If we inserted a GBA ROM with SRAM...
        _gbaSaveManager = std::make_optional<sram::SaveManager>(Console->GetGBASaveLength());
        retro::task::push(FlushGbaSramTask());
        retro::debug("Initialized and loaded GBA SRAM, and started GBA SRAM flush task.");
    }
    else {
        retro::info("No GBA SRAM was provided.");
    }

    if (retro::supports_power_status()) {
        retro::task::push(PowerStatusUpdateTask());
    }

    if (optional<unsigned> version = retro::message_interface_version(); version && version >= 1) {
        // If the frontend supports on-screen notifications...
        retro::task::push(OnScreenDisplayTask());
    }

    retro::environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)&MelonDsDs::input_descriptors);

    InitFlushFirmwareTask();

    if (_renderState.GetRenderMode() == RenderMode::OpenGl) {
        retro::info("Deferring initialization until the OpenGL context is ready");
        _deferredInitializationPending = true;
    }
    else {
        retro::info("No need to defer initialization, proceeding now");
        StartConsole();
    }

    return true;
}
catch (const config_exception& e) {
    retro::error("{}", e.what());

    return InitErrorScreen(e);
}
catch (const emulator_exception& e) {
    // Thrown for invalid ROMs
    retro::error("{}", e.what());
    retro::set_error_message(e.user_message());
    return false;
}
catch (const std::exception& e) {
    retro::error("{}", e.what());
    retro::set_error_message(INTERNAL_ERROR_MESSAGE);
    return false;
}
catch (...) {
    retro::set_error_message(UNKNOWN_ERROR_MESSAGE);
    return false;
}

// Reset for the next time
void MelonDsDs::CoreState::UninstallDsiware(melonDS::DSi_NAND::NANDImage& nand) noexcept {
    ZoneScopedN(TracyFunction);

    if (!_ndsInfo) return;

    retro_assert(nand);

    const auto& header = *reinterpret_cast<const melonDS::NDSHeader*>(_ndsInfo->GetData().data());
    retro_assert(header.IsDSiWare());

    if (NANDMount mount = NANDMount(nand)) {
        // TODO: Report an error if the title doesn't exist
        // TODO: Only delete the title if the sentinel exists
        ExportDsiwareSaveData(mount, *_ndsInfo, header, TitleData_PublicSav);
        ExportDsiwareSaveData(mount, *_ndsInfo, header, TitleData_PrivateSav);
        ExportDsiwareSaveData(mount, *_ndsInfo, header, TitleData_BannerSav);

        mount.DeleteTitle(header.DSiTitleIDHigh, header.DSiTitleIDLow);
        retro::info("Removed temporarily-installed DSiWare title \"{}\" from NAND image", _ndsInfo->GetPath());
    } else {
        retro::error("Failed to open DSi NAND for uninstallation");
    }
}

void MelonDsDs::CoreState::ExportDsiwareSaveData(NANDMount& nand, const retro::GameInfo& nds_info, const melonDS::NDSHeader& header, int type) noexcept {
    ZoneScopedN(TracyFunction);

    if (type == TitleData_PublicSav && header.DSiPublicSavSize == 0) {
        // If there's no public save data...
        retro::info("Game does not use public save data");
        return;
    }

    if (type == TitleData_PrivateSav && header.DSiPrivateSavSize == 0) {
        // If this game doesn't use private save data...
        retro::info("Game does not use private save data");
        return;
    }

    if (type == TitleData_BannerSav && !(header.AppFlags & 0x4)) {
        // If there's no banner save data...
        retro::info("Game does not use banner save data");
        return;
    }

    char sav_file[PATH_MAX]; // "/path/to/game.zip#game.nds"
    if (!GetDsiwareSaveDataHostPath(sav_file, nds_info, type)) {
        return;
    }

    if (nand.ExportTitleData(header.DSiTitleIDHigh, header.DSiTitleIDLow, type, sav_file)) {
        retro::info("Exported DSiWare save data to \"{}\"", sav_file);
    } else {
        retro::warn("Couldn't export DSiWare save data to \"{}\"", sav_file);
    }
}

void MelonDsDs::CoreState::ApplyConfig(const CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    MicInputMode oldMicInputMode = config.MicInputMode();

    std::optional<RenderMode> oldRenderer = _renderState.GetRenderMode();
    _renderState.Apply(config);
    _screenLayout.Apply(config, _renderState);
    _inputState.Apply(config);
    _micState.Apply(config);
    _screenLayout.SetDirty();

    if (oldMicInputMode != MicInputMode::HostMic && config.MicInputMode() == MicInputMode::HostMic) {
        // If we want to use the host's microphone, and we're coming from another setting...
        // (so that excessive warnings aren't shown)
        if (!_micState.IsMicInterfaceAvailable() && config.ShowUnsupportedFeatureWarnings()) {
            // ...but this frontend doesn't support it...
            retro::set_warn_message("This frontend doesn't support microphones.");
        }
        else if (!_micState.IsHostMicOpen()) {
            retro::warn("Failed to open host microphone");
        }
    }

    std::optional<RenderMode> newRenderer = _renderState.GetRenderMode();

    if (oldRenderer && newRenderer) {
        // If this isn't the first time we're setting the renderer...
        if (oldRenderer != newRenderer) {
            // If we're switching renderer modes...
            retro_system_av_info av = GetSystemAvInfo(*newRenderer);
            retro::set_system_av_info(av);
        }

        _renderState.UpdateRenderer(Config, *Console);
        _screenLayout.SetDirty();
    }
}

void MelonDsDs::CoreState::InitContent(unsigned type, std::span<const retro_game_info> game) {
    ZoneScopedN(TracyFunction);

    // First initialize the content info...
    switch (type) {
        case MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT:
            if (game.size() > 2 && game[2].path != nullptr) {
                // If we got a GBA SRAM file...
                _gbaSaveInfo = game[2];
            }

            [[fallthrough]];
        case MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT_NO_SRAM:
            if (game.size() > 1) {
                // If we got a GBA ROM...
                retro_assert(game[1].data != nullptr);
                _gbaInfo = game[1];
            }

            [[fallthrough]];
        case MELONDSDS_GAME_TYPE_NDS:
            if (!game.empty()) {
                retro_assert(game[0].data != nullptr);
                _ndsInfo = game[0];
            }
            break;
        default:
            retro::error("Unknown game type {}", type);
            retro::set_error_message(INTERNAL_ERROR_MESSAGE);
            throw std::runtime_error("Unknown game type");
    }
}

bool MelonDsDs::CoreState::UpdateOptionVisibility() noexcept {
    return _optionVisibility.Update();
}

/// Savestates in melonDS can vary in size depending on the game,
/// so we have to try saving the state first before we can know how big it'll be.
/// RetroArch may try to call this function before the ROM is installed
/// if rewind mode is enabled
size_t MelonDsDs::CoreState::SerializeSize() const noexcept {
    ZoneScopedN(TracyFunction);
    if (_messageScreen)
        return 0;
    // If there's an error, there's nothing to serialize

    if (!_savestateSize.has_value()) {
        // If we haven't yet figured out how big the savestate should be...

        retro_assert(Console != nullptr);
        if (static_cast<ConsoleType>(Console->ConsoleType) == ConsoleType::DSi) {
            // DSi mode doesn't support savestates right now
            _savestateSize = 0;
            // TODO: When DSi mode supports savestates, remove this conditional block
        }
        else {
#ifndef NDEBUG
            if (_ndsInfo) {
                // If we're booting with a ROM...

                // Savestate size varies by several factors, but SRAM length is the big one.
                // We won't know the size of the cart's SRAM until it's loaded,
                // so we can't know the savestate size until then.
                // We must ensure the cart is loaded before the frontend starts to ask about the savestate size!
                retro_assert(Console->NDSCartSlot.GetCart() != nullptr);
            }
#endif

            melonDS::Savestate state;
            Console->DoSavestate(&state);
            size_t length = state.Length();
            _savestateSize = length;

            retro::info(
                "Savestate requires {}B = {}KiB = {}MiB (before compression)",
                length,
                length / 1024.0f,
                length / 1024.0f / 1024.0f
            );
        }
    }

    retro_assert(_savestateSize.has_value());

    return *_savestateSize;
}

bool MelonDsDs::CoreState::Serialize(std::span<std::byte> data) const noexcept {
    ZoneScopedN(TracyFunction);
    if (_messageScreen)
        return false;

    retro_assert(Console != nullptr);

#ifndef NDEBUG
    if (_ndsInfo) {
        // If we're booting with a ROM...
        retro_assert(Console->GetNDSCart() != nullptr);
    }
#endif
    if (static_cast<ConsoleType>(Console->ConsoleType) == ConsoleType::DSi) {
        // DSi mode doesn't support savestates right now
        retro::error("DSi mode does not support saving states");
        return false;
    }

    if (_savestateSize) {
        // If we know how big the savestate for this game should be...
        melonDS::Savestate state(data.data(), data.size(), true);

        return Console->DoSavestate(&state) && !state.Error;
    }

    melonDS::Savestate state;
    Console->DoSavestate(&state);
    size_t length = state.Length();
    _savestateSize = length;

    if (_savestateSize != data.size()) {
        retro::error("Expected to save a {}-byte savestate, got a {}-byte buffer", *_savestateSize, data.size());
        return false;
    }

    memcpy(data.data(), state.Buffer(), state.Length());
    return true;
}

bool MelonDsDs::CoreState::Unserialize(std::span<const std::byte> data) noexcept {
    ZoneScopedN(TracyFunction);
    if (_messageScreen)
        return false;

    retro_assert(Console != nullptr);

#ifndef NDEBUG
    if (_ndsInfo.has_value()) {
        // If we're booting with a ROM...
        retro_assert(Console->GetNDSCart() != nullptr);
    }
#endif

    if (static_cast<ConsoleType>(Console->ConsoleType) == ConsoleType::DSi) {
        // DSi mode doesn't support savestates right now
        retro::error("DSi mode does not support loading states");
        return false;
    }

    if (!_savestateSize) {
        // If the frontend hasn't asked us about the savestate size yet...
        _savestateSize = SerializeSize();
    }

    if (data.size() != _savestateSize) {
        retro::error("Expected to load a {}-byte savestate, got {} bytes", *_savestateSize, data.size());
        return false;
    }

    melonDS::Savestate savestate(const_cast<void*>(static_cast<const void*>(data.data())), data.size(), false);

    if (savestate.Error) {
        uint16_t major = savestate.MajorVersion();
        uint16_t minor = savestate.MinorVersion();
        retro::error("Expected a savestate of major version {}, got {}.{}", SAVESTATE_MAJOR, major, minor);

        if (major < SAVESTATE_MAJOR) {
            // If this savestate is too old...
            retro::set_error_message(
                "This savestate is too old, can't load it.\n"
                "Save your game normally in the older version and import the save data.");
        }
        else if (major > SAVESTATE_MAJOR) {
            // If this savestate is too new...
            retro::set_error_message(
                "This savestate is too new, can't load it.\n"
                "Save your game normally in the newer version, "
                "then update this core or import the save data.");
        }

        return false;
    }

    if (data.size() != *_savestateSize) {
        retro::error("Expected a {}-byte savestate, got one of {} bytes", *_savestateSize, data.size());
        retro::set_error_message("Can't load this savestate, most likely the ROM or the core is wrong.");
        return false;
    }

    return Console->DoSavestate(&savestate) && !savestate.Error;
}

std::byte* MelonDsDs::CoreState::GetMemoryData(unsigned id) noexcept {
    ZoneScopedN(TracyFunction);
    if (_messageScreen)
        return nullptr;

    switch (id) {
        case RETRO_MEMORY_SYSTEM_RAM:
            retro_assert(Console != nullptr);
            return reinterpret_cast<std::byte*>(Console->MainRAM);
        case RETRO_MEMORY_SAVE_RAM:
            return _ndsSaveManager ? reinterpret_cast<std::byte*>(_ndsSaveManager->Sram()) : nullptr;
        default:
            return nullptr;
    }
}

size_t MelonDsDs::CoreState::GetMemorySize(unsigned id) noexcept {
    if (_messageScreen)
        return 0;

    switch (id) {
        case RETRO_MEMORY_SYSTEM_RAM: {
            retro_assert(Console != nullptr);
            auto consoleType = static_cast<ConsoleType>(Console->ConsoleType);
            switch (consoleType) {
                default:
                    retro::warn("Unknown console type {}, returning memory size of 4MB.", consoleType);
                    [[fallthrough]];
                // Intentional fall-through
                case ConsoleType::DS:
                    return DS_MEMORY_SIZE; // 4MB, the size of the DS system RAM
                case ConsoleType::DSi:
                    return melonDS::MainRAMMaxSize; // 16MB, the size of the DSi system RAM
            }
        }
        case RETRO_MEMORY_SAVE_RAM:
            return _ndsSaveManager ? _ndsSaveManager->SramLength() : 0;
        default:
            return 0;
    }
}

void MelonDsDs::CoreState::CheatSet(unsigned index, bool enabled, std::string_view code) noexcept {
    // Cheat codes are small programs, so we can't exactly turn them off (that would be undoing them)
    ZoneScopedN(TracyFunction);
    retro::debug("retro_cheat_set({}, {}, {})\n", index, enabled, code);
    if (code.empty())
        return;

    if (!enabled) {
        retro::set_warn_message("Action Replay codes can't be undone, restart the game to remove their effects.");
        return;
    }

    if (!regex_match(code.data(), _cheatSyntax)) {
        // If we're trying to activate this cheat code, but it's not valid...
        retro::set_warn_message("Cheat #{} ({:.8}...) isn't valid, ignoring it.", index, code);
        return;
    }

    melonDS::ARCode curcode {
        .Name = "",
        .Enabled = enabled,
        .Code = {}
    };

    // NDS cheats are sequence of unsigned 32-bit integers, each of which is hex-encoded
    auto end = std::cregex_iterator();
    for (auto i = std::cregex_iterator(code.cbegin(), code.cend(), _tokenSyntax); i != end; ++i) {
        const std::csub_match& match = (*i)[0];
        retro_assert(match.matched);
        uint32_t token = 0;
        std::from_chars_result result = std::from_chars(match.first, match.second, token, 16);
        retro_assert(result.ec == std::errc());
        curcode.Code.push_back(token);
    }

    retro_assert(Console != nullptr);
    Console->AREngine.RunCheat(curcode);
}
