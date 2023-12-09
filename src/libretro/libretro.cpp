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
#include <frontend/FrontendUtil.h>
#include <GBACart.h>
#include <NDS.h>
#include <NDSCart.h>
#include <SPI.h>
#undef isnan
#include <fmt/format.h>

#include "config.hpp"
#include "core.hpp"
#include "dsi.hpp"
#include "environment.hpp"
#include "exceptions.hpp"
#include "file.hpp"
#include "info.hpp"
#include "input.hpp"
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

    retro::task::TaskSpec OnScreenDisplayTask() noexcept;
}

PUBLIC_SYMBOL void retro_init(void) {
#ifdef HAVE_TRACY
    tracy::StartupProfiler();
#endif
    TracySetProgramName(MELONDSDS_VERSION_STRING);
    ZoneScopedN(TracyFunction);
    retro::env::init();
    retro::debug("retro_init");
    retro::info("{} {}", MELONDSDS_NAME, MELONDSDS_VERSION);
    retro_assert(!MelonDsDs::Core.IsInitialized());
    retro_assert(MelonDsDs::Core.Console == nullptr);

    retro::task::init(false, nullptr);

    new(&MelonDsDs::Core) MelonDsDs::CoreState(true); // placement-new the CoreState
    retro_assert(MelonDsDs::Core.IsInitialized());
}



static bool MelonDsDs::handle_load_game(unsigned type, const struct retro_game_info *info, size_t num) noexcept try {
    ZoneScopedN("MelonDsDs::handle_load_game");

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
    ZoneScopedN(TracyFunction);
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
        ZoneScopedN(TracyFunction);
        MelonDsDs::Core.Run();
    }
    FrameMark;
}

PUBLIC_SYMBOL void retro_unload_game(void) {
    ZoneScopedN(TracyFunction);
    using MelonDsDs::Core;
    retro::debug("retro_unload_game()");
    // No need to flush SRAM to the buffer, Platform::WriteNDSSave has been doing that for us this whole time
    // No need to flush the homebrew save data either, the CartHomebrew destructor does that

    // The cleanup handlers for each task will flush data to disk if needed
    retro::task::reset();
    retro::task::wait();
    retro::task::deinit();

    Core.UnloadGame();
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
    { // Scoped so that we can capture one last scope before shutting down the profiler
        ZoneScopedN(TracyFunction);
        retro::debug("retro_deinit()");
        retro::task::deinit();
        MelonDsDs::Core.~CoreState(); // placement delete
        retro_assert(!MelonDsDs::Core.IsInitialized());
        retro_assert(MelonDsDs::Core.Console == nullptr);
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

void MelonDsDs::HardwareContextReset() noexcept {
    try {
        Core.ResetRenderState();
    }
    catch (const opengl_exception& e) {
        retro::error("{}", e.what());
        retro::set_error_message(e.user_message());
        retro::shutdown();
        // TODO: Instead of shutting down, fall back to the software renderer
    }
    catch (const emulator_exception& e) {
        retro::error("{}", e.what());
        retro::set_error_message(e.user_message());
        retro::shutdown();
    }
    catch (const std::exception& e) {
        retro::set_error_message(e.what());
        retro::shutdown();
    }
    catch (...) {
        retro::set_error_message("OpenGL context initialization failed with an unknown error. Please report this issue.");
        retro::shutdown();
    }
}

void MelonDsDs::HardwareContextDestroyed() noexcept {
    Core.DestroyRenderState();
}

static void MelonDsDs::load_games(
    const optional<struct retro_game_info> &nds_info,
    const optional<struct retro_game_info> &gba_info,
    const optional<struct retro_game_info> &gba_save_info
) {
    ZoneScopedN("MelonDsDs::load_games");
    using MelonDsDs::Core;

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
