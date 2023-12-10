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

#include <libretro.h>
#include <retro_assert.h>

#include <NDS.h>
#include <compat/strl.h>
#include <file/file_path.h>

#include "dsi.hpp"
#include "exceptions.hpp"
#include "info.hpp"
#include "microphone.hpp"
#include "message/error.hpp"
#include "render/render.hpp"
#include "retro/task_queue.hpp"

using std::byte;
using std::span;

constexpr size_t DS_MEMORY_SIZE = 0x400000;
constexpr size_t DSI_MEMORY_SIZE = 0x1000000;
static const char* const INTERNAL_ERROR_MESSAGE =
    "An internal error occurred with melonDS DS. "
    "Please contact the developer with the log file.";

static const char* const UNKNOWN_ERROR_MESSAGE =
    "An unknown error has occurred with melonDS DS. "
    "Please contact the developer with the log file.";

retro_system_av_info MelonDsDs::CoreState::GetSystemAvInfo() const noexcept {
#ifndef NDEBUG
    if (!_messageScreen) {
        retro_assert(Console != nullptr);
    }
#endif

    return {
        .timing {
            .fps = 32.0f * 1024.0f * 1024.0f / 560190.0f,
            .sample_rate = 32.0f * 1024.0f,
        },
        .geometry = _screenLayout.Geometry(Console->GPU.GetRenderer3D()),
    };
}


void MelonDsDs::CoreState::UnloadGame() noexcept {
    if (Console && Console->IsRunning()) {
        // If the NDS wasn't already stopped due to some internal event...
        Console->Stop();
    }

    if (_ndsInfo) {
        // If this session involved a loaded DS game...

        retro_assert(_ndsInfo->GetData().size() > 0);
        const melonDS::NDSHeader& header = *reinterpret_cast<const melonDS::NDSHeader*>(_ndsInfo->GetData().data());
        if (header.IsDSiWare()) {
            // And that game was a DSiWare game...
            retro_assert(Console->ConsoleType == 1);
            retro_assert(dynamic_cast<melonDS::DSi*>(Console.get()) != nullptr);

            melonDS::DSi& dsi = *static_cast<melonDS::DSi*>(Console.get());
            // DSiWare "cart" shouldn't have been cleaned up yet
            // (a regular DS cart would've been moved-from at the start of the session)
            UninstallDsiware(dsi.GetNAND());
        }
    }

    Core.Console = nullptr;
}

void MelonDsDs::CoreState::Run() noexcept {
    ZoneScopedN(TracyFunction);

    if (_deferredInitializationPending && !RunDeferredInitialization()) [[unlikely]] {
        // If we needed to run any extra setup, but that process failed...
        retro::shutdown();
        return;
    }

    if (_messageScreen) [[unlikely]] {
        _messageScreen->Render(_screenLayout);
        return;
    }

    retro_assert(Console != nullptr);
    melonDS::NDS& nds = *Console;

    if (!_firstFrameRun) [[unlikely]] {
        RunFirstFrame();
    }

    if (retro::is_variable_updated()) {
        // If any settings have changed...
        MelonDsDs::UpdateConfig(MelonDsDs::Core, _screenLayout, _inputState);
    }

    if (_renderState->Ready()) [[likely]] {
        // If the global state needed for rendering is ready...
        ZoneScopedN("retro_run::render");
        HandleInput(nds, _inputState, _screenLayout);
        ReadMicrophone(nds, _inputState);

        if (_screenLayout.Dirty()) {
            // If the active screen layout has changed (either by settings or by hotkey)...
            ZoneScopedN("retro_run::render::dirty");

            // Apply the new screen layout
            _screenLayout.Update(Config.ScreenFilter());

            // And update the geometry
            if (!retro::set_geometry(_screenLayout.Geometry(Console->GPU.GetRenderer3D()))) {
                retro::warn("Failed to update geometry after screen layout change");
            }

            _renderState->RequestRefresh();
        }

        // NDS::RunFrame renders the Nintendo DS state to a framebuffer,
        // which is then drawn to the screen by MelonDsDs::render::Render
        {
            ZoneScopedN("NDS::RunFrame");
            nds.RunFrame();
        }

        _renderState->Render(nds, _inputState, Config, _screenLayout);
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

    auto header = _ndsInfo ? reinterpret_cast<const melonDS::NDSHeader*>(_ndsInfo->GetData().data()) : nullptr;
    retro_assert(Console != nullptr);
    InitConfig(*Console, header, _screenLayout, _inputState);

    InitFlushFirmwareTask();

    if (_ndsInfo) {
        // We need to reload the ROM because it might need to be encrypted with a different key,
        // depending on which console mode and BIOS mode is in effect.
        std::unique_ptr<melonDS::NDSCart::CartCommon> rom;
        {
            span<const byte> romSpan = _ndsInfo->GetData();
            ZoneScopedN("NDSCart::ParseROM");
            rom = melonDS::NDSCart::ParseROM(reinterpret_cast<const uint8_t*>(romSpan.data()), romSpan.size());
        }
        if (rom->GetSaveMemory()) {
            retro_assert(rom->GetSaveMemoryLength() == Console->GetNDSSaveLength());
            memcpy(rom->GetSaveMemory(), Console->GetNDSSave(), Console->GetNDSSaveLength());
        }

        {
            ZoneScopedN("NDSCart::InsertROM");
            Console->SetNDSCart(std::move(rom));
        }
        // TODO: Only reload the ROM if the BIOS mode, boot mode, or console mode has changed
    }

    Console->Reset();

    SetConsoleTime(*Console);
    if (_ndsInfo && Console->GetNDSCart() && !Console->GetNDSCart()->GetHeader().IsDSiWare()) {
        SetUpDirectBoot(*Console, *_ndsInfo);
    }

    _firstFrameRun = false;
}


void MelonDsDs::CoreState::RenderAudio(melonDS::NDS& nds) {
    ZoneScopedN("MelonDsDs::render_audio");
    int16_t audio_buffer[0x1000]; // 4096 samples == 2048 stereo frames
    uint32_t size = std::min(nds.SPU.GetOutputSize(), static_cast<int>(sizeof(audio_buffer) / (2 * sizeof(int16_t))));
    // Ensure that we don't overrun the buffer

    size_t read = nds.SPU.ReadOutput(audio_buffer, size);
    retro::audio_sample_batch(audio_buffer, read);
}

bool MelonDsDs::CoreState::RunDeferredInitialization() noexcept {
    ZoneScopedN(TracyFunction);
    retro_assert(MelonDsDs::Core.Console != nullptr);
    try {
        retro::debug("Starting deferred initialization");
        LoadGameDeferred();
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
        retro::error("Skipping error screen due to the environment variable MELONDSDS_SKIP_ERROR_SCREEN");
        return false;
    }

    retro::task::reset();
    _messageScreen = make_unique<error::ErrorScreen>(e);
    _screenLayout.Update(Config.ScreenFilter());
    retro::error("Error screen initialized");
    return true;
}

void MelonDsDs::CoreState::RunFirstFrame() noexcept {
    ZoneScopedN(TracyFunction);

    if (_firstFrameRun) return;

    // Apply the save data from the core's SRAM buffer to the cart's SRAM;
    // we need to do this in the first frame of retro_run because
    // retro_get_memory_data is used to copy the loaded SRAM
    // in between retro_load and the first retro_run call.

    // Nintendo DS SRAM is loaded by the frontend
    // and copied into NdsSaveManager via the pointer returned by retro_get_memory.
    // This is where we install the SRAM data into the emulated DS.
    if (_ndsInfo && _ndsSaveManager && _ndsSaveManager->SramLength() > 0) {
        // If we're loading a NDS game that has SRAM...
        ZoneScopedN("NDS::LoadSave");
        Console->SetNDSSave(_ndsSaveManager->Sram(), _ndsSaveManager->SramLength());
    }

    // GBA SRAM is selected by the user explicitly (due to libretro limits) and loaded by the frontend,
    // but is not processed by retro_get_memory (again due to libretro limits).
    if (_gbaInfo && _gbaSaveManager && _gbaSaveManager->SramLength() > 0) {
        // If we're loading a GBA game that has existing SRAM...
        ZoneScopedN("GBACart::LoadSave");
        // TODO: Decide what to do about SRAM files that append extra metadata like the RTC
        Console->SetGBASave(_gbaSaveManager->Sram(), _gbaSaveManager->SramLength());
    }

    // We could've installed the GBA's SRAM in retro_load_game (since it's not processed by retro_get_memory),
    // but doing so here helps keep things tidier since the NDS SRAM is installed here too.

    // This has to be deferred even if we're not using OpenGL,
    // because libretro doesn't set the SRAM until after retro_load_game
    _firstFrameRun = true;
}

void MelonDsDs::CoreState::SetConsoleTime(melonDS::NDS& nds) noexcept {
    ZoneScopedN(TracyFunction);
    time_t now = time(nullptr);
    tm tm;
    struct tm* tmPtr = localtime(&now);
    memcpy(&tm, tmPtr, sizeof(tm)); // Reduce the odds of race conditions in case some other thread uses this
    nds.RTC.SetDateTime(tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    // tm.tm_mon is 0-indexed, but RTC::SetDateTime expects 1-indexed
}

// melonDS tightly couples the renderer with the rest of the emulation code,
// so we can't initialize the emulator until the OpenGL context is ready.
void MelonDsDs::CoreState::LoadGameDeferred() {
    ZoneScopedN(TracyFunction);

    retro_assert(Console != nullptr); // This function should only be called if the console is initialized

    {
        ZoneScopedN("NDS::Reset");
        Console->Reset();
    }

    SetConsoleTime(*Console);

    if (_ndsInfo && Console->GetNDSCart() && !Console->GetNDSCart()->GetHeader().IsDSiWare()) {
        SetUpDirectBoot(*Console, *_ndsInfo);
    }

    Console->Start();

    retro::info("Initialized emulated console and loaded emulated game");
}

// Decrypts the ROM's secure area
void MelonDsDs::CoreState::SetUpDirectBoot(melonDS::NDS& nds, const retro::GameInfo& game) noexcept {
    ZoneScopedN(TracyFunction);
    if (Config.BootMode() == BootMode::Direct || Console->NeedsDirectBoot()) {
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



void MelonDsDs::CoreState::ReadMicrophone(melonDS::NDS& nds, MelonDsDs::InputState& inputState) noexcept {
    ZoneScopedN(TracyFunction);
    MicInputMode mic_input_mode = Config.MicInputMode();
    MicButtonMode mic_button_mode = Config.MicButtonMode();
    bool should_mic_be_on = false;

    switch (mic_button_mode) {
        // If the microphone button...
        case MicButtonMode::Hold: {
            // ...must be held...
            _micStateToggled = false;
            if (!inputState.MicButtonDown()) {
                // ...but it isn't...
                mic_input_mode = MicInputMode::None;
            }
            should_mic_be_on = inputState.MicButtonDown();
            break;
        }
        case MicButtonMode::Toggle: {
            // ...must be toggled...
            if (inputState.MicButtonPressed()) {
                // ...but it isn't...
                _micStateToggled = !_micStateToggled;
                if (!_micStateToggled) {
                    mic_input_mode = MicInputMode::None;
                }
            }
            should_mic_be_on = _micStateToggled;
            break;
        }
        case MicButtonMode::Always: {
            // ...is unnecessary...
            // Do nothing, the mic input mode is already set
            should_mic_be_on = true;
            break;
        }
    }

    if (retro::microphone::is_open()) {
        // TODO: Don't set constantly, only when the mic button state changes (or when the input mode is set to Always)
        retro::microphone::set_state(should_mic_be_on);
    }

    switch (mic_input_mode) {
        case MicInputMode::WhiteNoise: // random noise
        {
            int16_t tmp[735];
            for (int i = 0; i < 735; i++) tmp[i] = rand() & 0xFFFF;
            nds.MicInputFrame(tmp, 735);
            break;
        }
        case MicInputMode::HostMic: // microphone input
        {
            const auto mic_state = retro::microphone::get_state();
            if (mic_state.has_value() && mic_state.value()) {
                // If the microphone is open and turned on...
                int16_t samples[735];
                retro::microphone::read(samples, std::size(samples));
                nds.MicInputFrame(samples, std::size(samples));
                break;
            }
            // If the mic isn't available, feed silence instead
        }
        // Intentional fall-through
        default:
            nds.MicInputFrame(nullptr, 0);
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

bool MelonDsDs::CoreState::LoadGame(unsigned type, std::span<const retro_game_info> game) noexcept try {
    ZoneScopedN(TracyFunction);

    InitContent(type, game);

    // ...then load the game.
    if (!retro::set_pixel_format(RETRO_PIXEL_FORMAT_XRGB8888)) {
        throw environment_exception(
            "Failed to set the required XRGB8888 pixel format for rendering; it may not be supported.");
    }

    retro_assert(Console == nullptr);

    span<const byte> rom = _ndsInfo ? _ndsInfo->GetData() : span<const byte>();
    const auto* header = _ndsInfo ? reinterpret_cast<const melonDS::NDSHeader*>(rom.data()) : nullptr;
    // TODO: Apply config
    InitConfig(header, _screenLayout, _inputState);

    retro_assert(Console != nullptr);

    melonDS::NDS& nds = *Console;

    if (retro::supports_power_status()) {
        retro::task::push(PowerStatusUpdateTask());
    }

    if (optional<unsigned> version = retro::message_interface_version(); version && version >= 1) {
        // If the frontend supports on-screen notifications...
        retro::task::push(OnScreenDisplayTask());
    }

    using retro::environment;
    using retro::set_message;

    std::unique_ptr<NdsCart> loadedNdsCart;
    // First parse the ROMs...
    if (_ndsInfo) {
        {
            ZoneScopedN("NDSCart::ParseROM");
            loadedNdsCart = melonDS::NDSCart::ParseROM(reinterpret_cast<const uint8_t*>(rom.data()), rom.size());
            // TODO: Create FATStorageArgs if the cart is homebrew
        }

        if (!loadedNdsCart) {
            throw invalid_rom_exception("Failed to parse the DS ROM image. Is it valid?");
        }

        retro::debug("Loaded NDS ROM: \"{}\"", _ndsInfo->GetPath());

        if (!loadedNdsCart->GetHeader().IsDSiWare()) {
            // If this ROM represents a cartridge, rather than DSiWare...
            sram::InitNdsSave(*loadedNdsCart);
        }
    }

    std::unique_ptr<GbaCart> loadedGbaCart;
    if (_gbaInfo) {
        if (static_cast<ConsoleType>(Console->ConsoleType) == ConsoleType::DSi) {
            retro::set_warn_message(
                "The DSi does not support GBA connectivity. Not loading the requested GBA ROM or SRAM.");
        }
        else {
            // TODO: Load the ROM and SRAM in one go

            span<const byte> gbaRom = _gbaInfo->GetData();
            {
                ZoneScopedN("GBACart::ParseROM");
                loadedGbaCart = melonDS::GBACart::ParseROM(reinterpret_cast<const uint8_t*>(gbaRom.data()), gbaRom.size());
            }

            if (!loadedGbaCart) {
                throw invalid_rom_exception("Failed to parse the GBA ROM image. Is it valid?");
            }

            retro::debug("Loaded GBA ROM: \"{}\"", _gbaInfo->GetPath());

            if (_gbaSaveInfo) {
                sram::InitGbaSram(*loadedGbaCart, *_gbaSaveInfo);
            }
            else {
                retro::info("No GBA SRAM was provided.");
            }
        }
    }

    InitFlushFirmwareTask();

    environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)&MelonDsDs::input_descriptors);

    InitRenderer();
    // render::Initialize(config::video::ConfiguredRenderer());

    // The ROM must be inserted in retro_load_game,
    // as the frontend may try to query the savestate size
    // in between retro_load_game and the first retro_run call.
    retro_assert(nds.NDSCartSlot.GetCart() == nullptr);

    if (loadedNdsCart) {
        // If we want to insert a NDS ROM that was previously loaded...

        if (!loadedNdsCart->GetHeader().IsDSiWare()) {
            // If we're running a physical cartridge...

            nds.SetNDSCart(std::move(loadedNdsCart));

            // Just to be sure
            retro_assert(loadedNdsCart == nullptr);
            retro_assert(nds.NDSCartSlot.GetCart() != nullptr);
        }
        else {
            retro_assert(Core.Console->ConsoleType == 1);
            auto& dsi = *static_cast<melonDS::DSi*>(Core.Console.get());
            // We're running a DSiWare game, then
            MelonDsDs::dsi::install_dsiware(dsi.GetNAND(), *_ndsInfo);
        }
    }

    if (_gbaInfo && loadedGbaCart) {
        // If we want to insert a GBA ROM that was previously loaded...
        nds.SetGBACart(std::move(loadedGbaCart));

        retro_assert(loadedGbaCart == nullptr);
    }

    if (Console->GPU.GetRenderer3D().Accelerated) {
        retro::info("Deferring initialization until the OpenGL context is ready");
        _deferredInitializationPending = true;
    }
    else {
        retro::info("No need to defer initialization, proceeding now");
        LoadGameDeferred();
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


void MelonDsDs::CoreState::InitContent(unsigned type, std::span<const retro_game_info> game) {
    ZoneScopedN(TracyFunction);

    // First initialize the content info...
    switch (type) {
        case MELONDSDS_GAME_TYPE_NDS:
            // ...which refers to a Nintendo DS game...
            if (game.size() > 0) {
                _ndsInfo = game[0];
            }
            break;
        case MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT:
            // ...which refers to both a Nintendo DS and Game Boy Advance game...
            switch (game.size()) {
                case 3: // NDS ROM, GBA ROM, and GBA SRAM
                    _gbaSaveInfo = game[2];
                    [[fallthrough]];
                case 2: // NDS ROM and GBA ROM
                    _ndsInfo = game[0];
                    _gbaInfo = game[1];
                    break;
                default:
                    retro::error("Invalid number of ROMs ({}) for slot-1/2 boot", game.size());
                    retro::set_error_message(INTERNAL_ERROR_MESSAGE);
                    throw std::runtime_error("Invalid number of ROMs for slot-1/2 boot");
                // TODO: Throw an exception
            }
            break;
        default:
            retro::error("Unknown game type {}", type);
            retro::set_error_message(INTERNAL_ERROR_MESSAGE);
            throw std::runtime_error("Unknown game type");
    }
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
    retro_assert(_savestateSize.has_value());
    retro_assert(data.size() == _savestateSize);

    melonDS::Savestate state(data.data(), data.size(), true);

    return Console->DoSavestate(&state) && !state.Error;
}

bool MelonDsDs::CoreState::Unserialize(std::span<const std::byte> data) noexcept {
    ZoneScopedN(TracyFunction);
    if (_messageScreen)
        return false;

    retro_assert(Console != nullptr);
    retro_assert(_savestateSize.has_value());

#ifndef NDEBUG
    if (_ndsInfo.has_value()) {
        // If we're booting with a ROM...
        retro_assert(Console->GetNDSCart() != nullptr);
    }
#endif

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

    if (data.size() != _savestateSize) {
        retro::error("Expected a {}-byte savestate, got one of {} bytes", _savestateSize, data.size());
        retro::set_error_message("Can't load this savestate, most likely the ROM or the core is wrong.");
        return false;
    }

    return Console->DoSavestate(&savestate) && !savestate.Error;
}

byte* MelonDsDs::CoreState::GetMemoryData(unsigned id) noexcept {
    ZoneScopedN(TracyFunction);
    if (_messageScreen)
        return nullptr;

    switch (id) {
        case RETRO_MEMORY_SYSTEM_RAM:
            retro_assert(Console != nullptr);
            return reinterpret_cast<byte*>(Console->MainRAM);
        case RETRO_MEMORY_SAVE_RAM:
            if (_ndsSaveManager) {
                return (byte*)_ndsSaveManager->Sram();
            }
            [[fallthrough]];
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
            if (_ndsSaveManager) {
                return _ndsSaveManager->SramLength();
            }
            [[fallthrough]];
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
