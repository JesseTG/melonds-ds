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

PUBLIC_SYMBOL bool retro_load_game(const struct retro_game_info *info) {
    ZoneScopedN(TracyFunction);
    if (info) {
        ZoneText(info->path, strlen(info->path));
        retro::debug("retro_load_game(\"{}\", {})", info->path, info->size);
    }
    else {
        retro::debug("retro_load_game(<no content>)");
    }

    return MelonDsDs::Core.LoadGame(MelonDsDs::MELONDSDS_GAME_TYPE_NDS, std::span(info, 1));
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
