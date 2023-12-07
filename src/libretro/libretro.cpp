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

#include "libretro.hpp"

// NOT UNUSED; GPU.h doesn't #include OpenGL, so I do it here.
// This must come before <GPU.h>!
#include "PlatformOGLPrivate.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>

#include <compat/strl.h>
#include <file/file_path.h>
#include <libretro.h>
#include <retro_assert.h>
#include <retro_miscellaneous.h>
#include <streams/rzip_stream.h>

#include <DSi.h>
#include <DSi_I2C.h>
#include <frontend/FrontendUtil.h>
#include <GBACart.h>
#include <GPU.h>
#include <NDS.h>
#include <NDSCart.h>
#include <Platform.h>
#include <SPI.h>
#include <SPU.h>
#include <ARM.h>
#undef isnan
#include <fmt/format.h>
#include <RTC.h>

#include "cheats.hpp"
#include "config.hpp"
#include "content.hpp"
#include "core.hpp"
#include "dsi.hpp"
#include "environment.hpp"
#include "message/error.hpp"
#include "exceptions.hpp"
#include "file.hpp"
#include "info.hpp"
#include "input.hpp"
#include "memory.hpp"
#include "microphone.hpp"
#include "opengl.hpp"
#include "power.hpp"
#include "render.hpp"
#include "retro/task_queue.hpp"
#include "screenlayout.hpp"
#include "sram.hpp"
#include "tracy.hpp"

using namespace melonDS;
using std::make_optional;
using std::optional;
using std::nullopt;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::make_unique;
using retro::task::TaskSpec;

namespace MelonDsDs {
    static InputState input_state;
    static bool mic_state_toggled = false;
    static bool isInDeinit = false;
    static bool isUnloading = false;
    static bool deferred_initialization_pending = false;
    static bool first_frame_run = false;
    static uint32_t flushTaskId = 0;
    static const char *const INTERNAL_ERROR_MESSAGE =
        "An internal error occurred with melonDS DS. "
        "Please contact the developer with the log file.";

    static const char *const UNKNOWN_ERROR_MESSAGE =
        "An unknown error has occurred with melonDS DS. "
        "Please contact the developer with the log file.";

    // functions for loading games
    static bool handle_load_game(unsigned type, const struct retro_game_info *info, size_t num) noexcept;
    static void load_games(
        const optional<retro_game_info> &nds_info,
        const optional<retro_game_info> &gba_info,
        const optional<retro_game_info> &gba_save_info
    );
    static void load_games_deferred(
        NDS& nds,
        const optional<retro_game_info>& nds_info,
        const optional<retro_game_info>& gba_info
    );
    static void set_up_direct_boot(NDS& nds, const retro_game_info &nds_info);

    // functions for running games
    static void read_microphone(NDS& nds, MelonDsDs::InputState& inputState) noexcept;
    static void render_audio(NDS& nds);
    static void InitFlushFirmwareTask() noexcept
    {
        string_view firmwareName = config::system::FirmwarePath(config::system::ConsoleType());
        if (TaskSpec flushTask = sram::FlushFirmwareTask(firmwareName)) {
            flushTaskId = flushTask.Identifier();
            retro::task::push(std::move(flushTask));
        }
        else {
            retro::set_error_message("System path not found, changes to firmware settings won't be saved.");
        }
    }


    bool IsUnloadingGame() noexcept
    {
        return isUnloading;
    }

    bool IsInDeinit() noexcept
    {
        return isInDeinit;
    }

    bool IsInErrorScreen() noexcept {
        return _messageScreen != nullptr;
    }
    retro::task::TaskSpec OnScreenDisplayTask() noexcept;
}

PUBLIC_SYMBOL void retro_init(void) {
#ifdef HAVE_TRACY
    tracy::StartupProfiler();
#endif
    TracySetProgramName(MELONDSDS_VERSION_STRING);
    ZoneScopedN("retro_init");
    retro::env::init();
    retro::debug("retro_init");
    retro::info("{} {}", MELONDSDS_NAME, MELONDSDS_VERSION);
    retro_assert(!MelonDsDs::Core.IsInitialized());
    retro_assert(MelonDsDs::Core.Console == nullptr);
    retro_assert(retro::content::get_loaded_nds_info() == nullopt);
    retro_assert(retro::content::get_loaded_gba_info() == nullopt);
    retro_assert(retro::content::get_loaded_gba_save_info() == nullopt);
    retro_assert(!MelonDsDs::first_frame_run);
    retro_assert(!MelonDsDs::deferred_initialization_pending);
    retro_assert(!MelonDsDs::isInDeinit);
    retro_assert(!MelonDsDs::isUnloading);
    retro_assert(!MelonDsDs::mic_state_toggled);
    retro_assert(MelonDsDs::flushTaskId == 0);
    retro_assert(MelonDsDs::_messageScreen == nullptr);
    srand(time(nullptr));
    MelonDsDs::input_state = MelonDsDs::InputState();
    MelonDsDs::sram::init();


    MelonDsDs::file::init();
    MelonDsDs::first_frame_run = false;
    new(&MelonDsDs::Core) MelonDsDs::CoreState(true); // placement-new the CoreState
    retro_assert(MelonDsDs::Core.IsInitialized());
    retro::task::init(false, nullptr);

    // ScreenLayoutData is initialized in its constructor
}

static bool InitErrorScreen(const MelonDsDs::config_exception& e) noexcept {
    using namespace MelonDsDs;
    ZoneScopedN("MelonDsDs::InitErrorScreen");
    retro_assert(MelonDsDs::_messageScreen == nullptr);
    if (getenv("MELONDSDS_SKIP_ERROR_SCREEN")) {
        retro::error("Skipping error screen due to the environment variable MELONDSDS_SKIP_ERROR_SCREEN");
        return false;
    }

    retro::task::reset();
    MelonDsDs::_messageScreen = make_unique<error::ErrorScreen>(e);
    screenLayout.Update(MelonDsDs::Renderer::Software);
    retro::error("Error screen initialized");
    return true;
}

static bool MelonDsDs::handle_load_game(unsigned type, const struct retro_game_info *info, size_t num) noexcept try {
    ZoneScopedN("MelonDsDs::handle_load_game");
    retro_assert(retro::content::get_loaded_nds_info() == nullopt);
    retro_assert(retro::content::get_loaded_gba_info() == nullopt);
    retro_assert(retro::content::get_loaded_gba_save_info() == nullopt);

    // First initialize the content info...
    switch (type) {
        case MelonDsDs::MELONDSDS_GAME_TYPE_NDS:
            // ...which refers to a Nintendo DS game...
            retro::content::set_loaded_content_info(info, nullptr);
            break;
        case MelonDsDs::MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT:
            // ...which refers to both a Nintendo DS and Game Boy Advance game...
            switch (num) {
                case 2: // NDS ROM and GBA ROM
                    retro::content::set_loaded_content_info(info, info + 1);
                    break;
                case 3: // NDS ROM, GBA ROM, and GBA SRAM
                    retro::content::set_loaded_content_info(info, info + 1, info + 2);
                    break;
                default:
                    retro::error("Invalid number of ROMs ({}) for slot-1/2 boot", num);
                    retro::set_error_message(MelonDsDs::INTERNAL_ERROR_MESSAGE);
            }
            break;
        default:
            retro::error("Unknown game type {}", type);
            retro::set_error_message(MelonDsDs::INTERNAL_ERROR_MESSAGE);
            return false;
    }

    // ...then load the game.
    MelonDsDs::load_games(
        retro::content::get_loaded_nds_info(),
        retro::content::get_loaded_gba_info(),
        retro::content::get_loaded_gba_save_info()
    );

    return true;
}
catch (const MelonDsDs::config_exception& e) {
    retro::error("{}", e.what());

    return InitErrorScreen(e);
}
catch (const MelonDsDs::emulator_exception &e) {
    // Thrown for invalid ROMs
    retro::error("{}", e.what());
    retro::set_error_message(e.user_message());
    return false;
}
catch (const std::exception &e) {
    retro::error("{}", e.what());
    retro::set_error_message(MelonDsDs::INTERNAL_ERROR_MESSAGE);
    return false;
}
catch (...) {
    retro::set_error_message(MelonDsDs::UNKNOWN_ERROR_MESSAGE);
    return false;
}

PUBLIC_SYMBOL bool retro_load_game(const struct retro_game_info *info) {
    ZoneScopedN("retro_load_game");
    if (info) {
        ZoneText(info->path, strlen(info->path));
        retro::debug("retro_load_game(\"{}\", {})", info->path, info->size);
    }
    else {
        retro::debug("retro_load_game(<no content>)");
    }

    return MelonDsDs::handle_load_game(MelonDsDs::MELONDSDS_GAME_TYPE_NDS, info, 1);
}

PUBLIC_SYMBOL void retro_get_system_av_info(struct retro_system_av_info *info) {
    ZoneScopedN(TracyFunction);

    retro_assert(info != nullptr);

    *info = MelonDsDs::Core.GetSystemAvInfo();
}

PUBLIC_SYMBOL [[gnu::hot]] void retro_run(void) {
    {
        ZoneScopedN("retro_run");
        using namespace MelonDsDs;

        if (deferred_initialization_pending) {
            ZoneScopedN("retro_run::deferred_initialization");
            try {
                retro_assert(MelonDsDs::Core.Console != nullptr);
                retro::debug("Starting deferred initialization");
                MelonDsDs::load_games_deferred(
                    *MelonDsDs::Core.Console,
                    retro::content::get_loaded_nds_info(),
                    retro::content::get_loaded_gba_info()
                );
                deferred_initialization_pending = false;

                retro::debug("Completed deferred initialization");
            }
            catch (const MelonDsDs::config_exception &e) {
                retro::error("Deferred initialization failed; displaying error screen");
                retro::error("{}", e.what());
                retro::set_error_message(e.user_message());
                if (!InitErrorScreen(e))
                    return;
            }
            catch (const MelonDsDs::emulator_exception &e) {
                retro::error("Deferred initialization failed; exiting core");
                retro::error("{}", e.what());
                retro::set_error_message(e.user_message());
                retro::shutdown();
                return;
            }
            catch (const std::exception &e) {
                retro::error("Deferred initialization failed; exiting core");
                retro::set_error_message(e.what());
                retro::shutdown();
                return;
            }
            catch (...) {
                retro::error("Deferred initialization failed; exiting core");
                retro::set_error_message(UNKNOWN_ERROR_MESSAGE);
                retro::shutdown();
                return;
            }
        }

        if (_messageScreen) {
            _messageScreen->Render(screenLayout);
            return;
        }

        retro_assert(MelonDsDs::Core.Console != nullptr);
        NDS& nds = *MelonDsDs::Core.Console;

        if (!first_frame_run) {
            ZoneScopedN("retro_run::first_frame");
            using namespace sram;
            // Apply the save data from the core's SRAM buffer to the cart's SRAM;
            // we need to do this in the first frame of retro_run because
            // retro_get_memory_data is used to copy the loaded SRAM
            // in between retro_load and the first retro_run call.

            // Nintendo DS SRAM is loaded by the frontend
            // and copied into NdsSaveManager via the pointer returned by retro_get_memory.
            // This is where we install the SRAM data into the emulated DS.
            if (retro::content::get_loaded_nds_info() && NdsSaveManager && NdsSaveManager->SramLength() > 0) {
                // If we're loading a NDS game that has SRAM...
                ZoneScopedN("NDS::LoadSave");
                nds.SetGBASave(NdsSaveManager->Sram(), NdsSaveManager->SramLength());
            }

            // GBA SRAM is selected by the user explicitly (due to libretro limits) and loaded by the frontend,
            // but is not processed by retro_get_memory (again due to libretro limits).
            if (retro::content::get_loaded_gba_info() && GbaSaveManager && GbaSaveManager->SramLength() > 0) {
                // If we're loading a GBA game that has existing SRAM...
                ZoneScopedN("GBACart::LoadSave");
                // TODO: Decide what to do about SRAM files that append extra metadata like the RTC
                nds.SetGBASave(GbaSaveManager->Sram(), GbaSaveManager->SramLength());
            }

            // We could've installed the GBA's SRAM in retro_load_game (since it's not processed by retro_get_memory),
            // but doing so here helps keep things tidier since the NDS SRAM is installed here too.

            // This has to be deferred even if we're not using OpenGL,
            // because libretro doesn't set the SRAM until after retro_load_game
            first_frame_run = true;
        }

        if (retro::is_variable_updated()) {
            // If any settings have changed...
            MelonDsDs::UpdateConfig(MelonDsDs::Core, screenLayout, input_state);
        }

        if (MelonDsDs::render::ReadyToRender(nds)) {
            // If the global state needed for rendering is ready...
            ZoneScopedN("retro_run::render");
            HandleInput(nds, input_state, screenLayout);
            read_microphone(nds, input_state);

            if (screenLayout.Dirty()) {
                // If the active screen layout has changed (either by settings or by hotkey)...
                ZoneScopedN("retro_run::render::dirty");
                Renderer renderer = MelonDsDs::render::CurrentRenderer();
                retro_assert(renderer != Renderer::None);

                // Apply the new screen layout
                screenLayout.Update(renderer);

                // And update the geometry
                if (!retro::set_geometry(screenLayout.Geometry(renderer))) {
                    retro::warn("Failed to update geometry after screen layout change");
                }

                MelonDsDs::opengl::RequestOpenGlRefresh();
            }

            // NDS::RunFrame renders the Nintendo DS state to a framebuffer,
            // which is then drawn to the screen by MelonDsDs::render::Render
            {
                ZoneScopedN("NDS::RunFrame");
                nds.RunFrame();
            }

            render::Render(nds, input_state, screenLayout);
            MelonDsDs::render_audio(nds);

            retro::task::check();
        }
    }
    FrameMark;
}

static void MelonDsDs::read_microphone(NDS& nds, MelonDsDs::InputState& inputState) noexcept {
    ZoneScopedN("MelonDsDs::read_microphone");
    MicInputMode mic_input_mode = config::audio::MicInputMode();
    MicButtonMode mic_button_mode = config::audio::MicButtonMode();
    bool should_mic_be_on = false;

    switch (mic_button_mode) {
        // If the microphone button...
        case MicButtonMode::Hold: {
            // ...must be held...
            mic_state_toggled = false;
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
                mic_state_toggled = !mic_state_toggled;
                if (!mic_state_toggled) {
                    mic_input_mode = MicInputMode::None;
                }
            }
            should_mic_be_on = mic_state_toggled;
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
            s16 tmp[735];
            for (int i = 0; i < 735; i++) tmp[i] = rand() & 0xFFFF;
            nds.MicInputFrame(tmp, 735);
            break;
        }
        case MicInputMode::HostMic: // microphone input
        {
            const auto mic_state = retro::microphone::get_state();
            if (mic_state.has_value() && mic_state.value()) {
                // If the microphone is open and turned on...
                s16 samples[735];
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

static void MelonDsDs::render_audio(NDS& nds) {
    ZoneScopedN("MelonDsDs::render_audio");
    int16_t audio_buffer[0x1000]; // 4096 samples == 2048 stereo frames
    u32 size = std::min(nds.SPU.GetOutputSize(), static_cast<int>(sizeof(audio_buffer) / (2 * sizeof(int16_t))));
    // Ensure that we don't overrun the buffer

    size_t read = nds.SPU.ReadOutput(audio_buffer, size);
    retro::audio_sample_batch(audio_buffer, read);
}

static void SetConsoleTime(NDS& nds) noexcept {
    ZoneScopedN(TracyFunction);
    time_t now = time(nullptr);
    tm tm;
    struct tm* tmPtr = localtime(&now);
    memcpy(&tm, tmPtr, sizeof(tm)); // Reduce the odds of race conditions in case some other thread uses this
    nds.RTC.SetDateTime(tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    // tm.tm_mon is 0-indexed, but RTC::SetDateTime expects 1-indexed
}

PUBLIC_SYMBOL void retro_unload_game(void) {
    ZoneScopedN("retro_unload_game");
    using MelonDsDs::Core;
    MelonDsDs::isUnloading = true;
    retro::debug("retro_unload_game()");
    // No need to flush SRAM to the buffer, Platform::WriteNDSSave has been doing that for us this whole time
    // No need to flush the homebrew save data either, the CartHomebrew destructor does that

    // The cleanup handlers for each task will flush data to disk if needed
    retro::task::reset();
    retro::task::wait();
    retro::task::deinit();

    retro_assert(Core.Console != nullptr);
    NDS& nds = *Core.Console;
    if (nds.IsRunning())
    { // If the NDS wasn't already stopped due to some internal event...
        nds.Stop();
    }

    // Must uninstall the DSiWare between NDS::Stop and NDS::DeInit;
    // before NDS::Stop the NAND may still be in use,
    // and after NDS::DeInit the NANDImage is destroyed.
    if (const optional<struct retro_game_info>& nds_info = retro::content::get_loaded_nds_info()) {
        // If this session involved a loaded DS game...

        retro_assert(nds_info->data != nullptr);
        const NDSHeader& header = *reinterpret_cast<const NDSHeader*>(nds_info->data);
        if (header.IsDSiWare()) {
            // And that game was a DSiWare game...
            retro_assert(nds.ConsoleType == 1);
            retro_assert(dynamic_cast<DSi*>(&nds) != nullptr);

            DSi& dsi = *static_cast<DSi*>(&nds);
            // DSiWare "cart" shouldn't have been cleaned up yet
            // (a regular DS cart would've been moved-from at the start of the session)
            MelonDsDs::dsi::uninstall_dsiware(dsi.GetNAND(), *nds_info);
        }
    }

    Core.Console = nullptr;

    MelonDsDs::isUnloading = false;
}

PUBLIC_SYMBOL unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

PUBLIC_SYMBOL bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
    ZoneScopedN("retro_load_game_special");
    retro::debug("retro_load_game_special({}, {}, {})", MelonDsDs::get_game_type_name(type), fmt::ptr(info), num);

    return MelonDsDs::handle_load_game(type, info, num);
}

// We deinitialize all these variables just in case the frontend doesn't unload the dynamic library.
// It might be keeping the library around for debugging purposes,
// or it might just be buggy.
PUBLIC_SYMBOL void retro_deinit(void) {
    {
        ZoneScopedN("retro_deinit");
        MelonDsDs::isInDeinit = true;
        retro::debug("retro_deinit()");
        retro::task::deinit();
        MelonDsDs::file::deinit();
        retro::content::clear();
        MelonDsDs::clear_memory_config();
        MelonDsDs::sram::deinit();
        MelonDsDs::mic_state_toggled = false;
        MelonDsDs::isUnloading = false;
        MelonDsDs::deferred_initialization_pending = false;
        MelonDsDs::first_frame_run = false;
        MelonDsDs::isInDeinit = false;
        MelonDsDs::flushTaskId = 0;
        MelonDsDs::_messageScreen = nullptr;
        MelonDsDs::Core.~CoreState(); // placement delete
        retro_assert(!MelonDsDs::Core.IsInitialized());
        retro_assert(MelonDsDs::Core.Console == nullptr);
        MelonDsDs::cheats::deinit();
        retro::env::deinit();
    }

#ifdef HAVE_TRACY
    tracy::ShutdownProfiler();
#endif
}

PUBLIC_SYMBOL unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

PUBLIC_SYMBOL void retro_get_system_info(struct retro_system_info *info) {
    info->library_name = MELONDSDS_NAME;
    info->block_extract = false;
    info->library_version = MELONDSDS_VERSION;
    info->need_fullpath = false;
    info->valid_extensions = "nds|ids|dsi";
}

// TODO: Catch any thrown exceptions (in case the config is bad) and quit if needed
PUBLIC_SYMBOL void retro_reset(void) {
    ZoneScopedN("retro_reset");
    retro::debug("retro_reset()\n");

    if (MelonDsDs::_messageScreen) {
        retro::set_error_message("Please follow the advice on this screen, then unload/reload the core.");
        return;
        // TODO: Allow the game to be reset from the error screen
        // (gotta reinitialize the DS here)
    }

    // Flush all data before resetting
    MelonDsDs::sram::reset();
    retro::task::find([](retro::task::TaskHandle& task) {
        if (task.Identifier() == MelonDsDs::flushTaskId) {
            // If this is the flush task we want to cancel...
            task.Cancel();
            return true;
        }
        return false; // Keep looking...
    });
    retro::task::check();
    MelonDsDs::clear_memory_config();

    const optional<struct retro_game_info>& nds_info = retro::content::get_loaded_nds_info();
    const NDSHeader* header = nds_info ? reinterpret_cast<const NDSHeader*>(nds_info->data) : nullptr;
    retro_assert(MelonDsDs::Core.Console != nullptr);
    NDS& nds = *MelonDsDs::Core.Console;
    MelonDsDs::InitConfig(MelonDsDs::Core, header, MelonDsDs::screenLayout, MelonDsDs::input_state);

    MelonDsDs::InitFlushFirmwareTask();

    if (nds_info) {
        // We need to reload the ROM because it might need to be encrypted with a different key,
        // depending on which console mode and BIOS mode is in effect.
        std::unique_ptr<NDSCart::CartCommon> rom;
        {
            ZoneScopedN("NDSCart::ParseROM");
            rom = NDSCart::ParseROM(static_cast<const u8*>(nds_info->data), nds_info->size);
        }
        if (rom->GetSaveMemory()) {
            retro_assert(rom->GetSaveMemoryLength() == nds.NDSCartSlot.GetSaveMemoryLength());
            memcpy(rom->GetSaveMemory(), nds.NDSCartSlot.GetSaveMemory(), nds.NDSCartSlot.GetSaveMemoryLength());
        }

        {
            ZoneScopedN("NDSCart::InsertROM");
            nds.SetNDSCart(std::move(rom));
        }
        // TODO: Only reload the ROM if the BIOS mode, boot mode, or console mode has changed
    }

    nds.Reset();

    SetConsoleTime(nds);
    if (nds.NDSCartSlot.GetCart() && !nds.NDSCartSlot.GetCart()->GetHeader().IsDSiWare()) {
        MelonDsDs::set_up_direct_boot(nds, nds_info.value());
    }

    MelonDsDs::first_frame_run = false;
}

static void MelonDsDs::load_games(
    const optional<struct retro_game_info> &nds_info,
    const optional<struct retro_game_info> &gba_info,
    const optional<struct retro_game_info> &gba_save_info
) {
    ZoneScopedN("MelonDsDs::load_games");
    using MelonDsDs::Core;
    MelonDsDs::clear_memory_config();

    if (!retro::set_pixel_format(RETRO_PIXEL_FORMAT_XRGB8888)) {
        throw environment_exception("Failed to set the required XRGB8888 pixel format for rendering; it may not be supported.");
    }

    retro_assert(Core.Console == nullptr);

    const NDSHeader* header = nds_info ? reinterpret_cast<const NDSHeader*>(nds_info->data) : nullptr;
    // TODO: Apply config
    MelonDsDs::InitConfig(Core, header, screenLayout, input_state);

    retro_assert(Core.Console != nullptr);

    melonDS::NDS& nds = *Core.Console;

    if (retro::supports_power_status())
    {
        retro::task::push(MelonDsDs::power::PowerStatusUpdateTask());
    }

    if (optional<unsigned> version = retro::message_interface_version(); version && version >= 1) {
        // If the frontend supports on-screen notifications...
        retro::task::push(MelonDsDs::OnScreenDisplayTask());
    }

    using retro::environment;
    using retro::set_message;

    std::unique_ptr<NdsCart> loadedNdsCart;
    // First parse the ROMs...
    if (nds_info) {
        {
            ZoneScopedN("NDSCart::ParseROM");
            loadedNdsCart = NDSCart::ParseROM(static_cast<const u8*>(nds_info->data), nds_info->size);
        }

        if (!loadedNdsCart) {
            throw invalid_rom_exception("Failed to parse the DS ROM image. Is it valid?");
        }

        retro::debug("Loaded NDS ROM: \"{}\"", nds_info->path);

        if (!loadedNdsCart->GetHeader().IsDSiWare()) {
            // If this ROM represents a cartridge, rather than DSiWare...
            sram::InitNdsSave(*loadedNdsCart);
        }
    }

    std::unique_ptr<GbaCart> loadedGbaCart;
    if (gba_info) {
        if (config::system::ConsoleType() == ConsoleType::DSi) {
            retro::set_warn_message("The DSi does not support GBA connectivity. Not loading the requested GBA ROM or SRAM.");
        } else {
            // TODO: Load the ROM and SRAM in one go
            {
                ZoneScopedN("GBACart::ParseROM");
                loadedGbaCart = GBACart::ParseROM(static_cast<const u8*>(gba_info->data), gba_info->size);
            }

            if (!loadedGbaCart) {
                throw invalid_rom_exception("Failed to parse the GBA ROM image. Is it valid?");
            }

            retro::debug("Loaded GBA ROM: \"{}\"", gba_info->path);

            if (gba_save_info) {
                sram::InitGbaSram(*loadedGbaCart, *gba_save_info);
            }
            else {
                retro::info("No GBA SRAM was provided.");
            }
        }
    }

    InitFlushFirmwareTask();

    if (loadedGbaCart && (nds.IsLoadedARM9BIOSBuiltIn() || nds.IsLoadedARM7BIOSBuiltIn() || nds.SPI.GetFirmwareMem()->IsLoadedFirmwareBuiltIn())) {
        // If we're using FreeBIOS and are trying to load a GBA cart...
        retro::set_warn_message(
            "FreeBIOS does not support GBA connectivity. "
            "Please install a native BIOS and enable it in the options menu."
        );
    }

    environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void *) &MelonDsDs::input_descriptors);

    render::Initialize(config::video::ConfiguredRenderer());

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
            auto& dsi = *static_cast<DSi*>(Core.Console.get());
            // We're running a DSiWare game, then
            MelonDsDs::dsi::install_dsiware(dsi.GetNAND(), *nds_info);
        }
    }

    if (gba_info && loadedGbaCart) {
        // If we want to insert a GBA ROM that was previously loaded...
        nds.SetGBACart(std::move(loadedGbaCart));

        retro_assert(loadedGbaCart == nullptr);
    }

    if (render::CurrentRenderer() == Renderer::OpenGl) {
        retro::info("Deferring initialization until the OpenGL context is ready");
        deferred_initialization_pending = true;
    } else {
        retro::info("No need to defer initialization, proceeding now");
        load_games_deferred(nds, nds_info, gba_info);
    }
}

// melonDS tightly couples the renderer with the rest of the emulation code,
// so we can't initialize the emulator until the OpenGL context is ready.
static void MelonDsDs::load_games_deferred(
    NDS& nds,
    const optional<retro_game_info>& nds_info,
    const optional<retro_game_info>& gba_info
) {
    ZoneScopedN("MelonDsDs::load_games_deferred");

    {
        ZoneScopedN("NDS::Reset");
        nds.Reset();
    }

    SetConsoleTime(nds);

    if (nds_info && nds.NDSCartSlot.GetCart() && !nds.NDSCartSlot.GetCart()->GetHeader().IsDSiWare()) {
        set_up_direct_boot(nds, *nds_info);
    }

    nds.Start();

    retro::info("Initialized emulated console and loaded emulated game");
}

// Decrypts the ROM's secure area
static void MelonDsDs::set_up_direct_boot(NDS& nds, const retro_game_info &nds_info) {
    ZoneScopedN("MelonDsDs::set_up_direct_boot");
    if (config::system::DirectBoot() || nds.NeedsDirectBoot()) {
        char game_name[256];
        const char *ptr = path_basename(nds_info.path);
        if (ptr)
            strlcpy(game_name, ptr, sizeof(game_name));
        else
            strlcpy(game_name, nds_info.path, sizeof(game_name));

        {
            ZoneScopedN("NDS::SetupDirectBoot");
            nds.SetupDirectBoot(game_name);
        }
        retro::debug("Initialized direct boot for \"{}\"", game_name);
    }
}

retro::task::TaskSpec MelonDsDs::OnScreenDisplayTask() noexcept {
    return retro::task::TaskSpec(
        [](retro::task::TaskHandle&) noexcept {
            using std::to_string;
            ZoneScopedN("MelonDsDs::OnScreenDisplayTask");
            constexpr const char* const OSD_DELIMITER = " || ";
            constexpr const char* const OSD_YES = "✔";
            constexpr const char* const OSD_NO = "✘";

            retro_assert(MelonDsDs::Core.Console != nullptr);
            NDS& nds = *MelonDsDs::Core.Console;

            // TODO: If an on-screen display isn't supported, finish the task
            fmt::memory_buffer buf;
            auto inserter = std::back_inserter(buf);

            if (config::osd::ShowPointerCoordinates()) {
                glm::i16vec2 pointerInput = input_state.PointerInput();
                glm::ivec2 joystick = input_state.JoystickTouchPosition();
                glm::ivec2 touch = input_state.PointerTouchPosition();
                fmt::format_to(
                    inserter,
                    "Pointer: ({}, {}) → ({}, {}) || Joystick: ({}, {})",
                    pointerInput.x, pointerInput.y,
                    touch.x, touch.y,
                    joystick.x, joystick.y
                );
            }

            if (config::osd::ShowMicState()) {
                optional<bool> mic_state = retro::microphone::get_state();

                if (mic_state && *mic_state) {
                    // If the microphone is open and turned on...
                    fmt::format_to(
                        inserter,
                        "{}{}",
                        buf.size() == 0 ? "" : OSD_DELIMITER,
                        (nds.NumFrames % 120 > 60) ? "●" : "○"
                    );
                    // Toggle between a filled circle and an empty one every 1.5 seconds
                    // (kind of like a blinking "recording" light)
                }
            }

            if (config::osd::ShowCurrentLayout()) {
                fmt::format_to(
                    inserter,
                    "{}Layout {}/{}",
                    buf.size() == 0 ? "" : OSD_DELIMITER,
                    screenLayout.LayoutIndex() + 1,
                    screenLayout.NumberOfLayouts()
                );
            }

            if (config::osd::ShowLidState() && nds.IsLidClosed()) {
                fmt::format_to(
                    inserter,
                    "{}Closed",
                    buf.size() == 0 ? "" : OSD_DELIMITER
                );
            }

            // fmt::format_to does not append a null terminator
            buf.push_back('\0');


            if (buf.size() > 0) {
                retro_message_ext message {
                    .msg = buf.data(),
                    .duration = 60,
                    .priority = 0,
                    .level = RETRO_LOG_DEBUG,
                    .target = RETRO_MESSAGE_TARGET_OSD,
                    .type = RETRO_MESSAGE_TYPE_STATUS,
                    .progress = -1
                };
                retro::set_message(message);
            }

        },
        nullptr,
        nullptr,
        retro::task::ASAP,
        "OnScreenDisplayTask"
    );
}