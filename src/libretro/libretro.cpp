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

#include <ctime>
#include <cstdlib>
#include <cstring>

#include <compat/strl.h>
#include <file/file_path.h>
#include <libretro.h>
#include <streams/file_stream.h>

#include <NDS.h>
#include <NDSCart.h>
#include <frontend/FrontendUtil.h>
#include <Platform.h>
#include <frontend/qt_sdl/Config.h>
#include <GPU.h>
#include <SPU.h>
#include <GBACart.h>
#include <retro_assert.h>

#include "opengl.hpp"
#include "environment.hpp"
#include "config.hpp"
#include "input.hpp"
#include "utils.hpp"
#include "info.hpp"
#include "screenlayout.hpp"
#include "memory.hpp"
#include "render.hpp"

using NDSCart::NDSCartData;
using GBACart::GBACartData;

namespace melonds {
    static std::string _base_directory;
    static std::string _save_directory;
    static const retro_game_info *_nds_game_info;
    static const retro_game_info *_gba_game_info;
    static bool swap_screen_toggled = false;
    static bool deferred_initialization_pending = false;
    static bool first_frame_run = false;
    static std::unique_ptr<NDSCartData> _loaded_nds_cart;
    static std::unique_ptr<GBACartData> _loaded_gba_cart;

    static void render_frame();

    static void render_audio();

    static bool load_games(const struct retro_game_info *nds_info, const struct retro_game_info *gba_info);

    static bool load_game_deferred(const struct retro_game_info *nds_info, const struct retro_game_info *gba_info);

    static void initialize_bios();

    static void set_up_direct_boot(const retro_game_info *nds_info);
}

const std::string &retro::base_directory() {
    return melonds::_base_directory;
}

const std::string &retro::save_directory() {
    return melonds::_save_directory;
}

PUBLIC_SYMBOL void retro_init(void) {
    retro::log(RETRO_LOG_DEBUG, "retro_init");
    const char *dir = nullptr;

    srand(time(nullptr));
    if (retro::environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
        melonds::_base_directory = dir;

    if (retro::environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
        melonds::_save_directory = dir;

    Platform::Init(0, nullptr);
    melonds::first_frame_run = false;
    // ScreenLayoutData is initialized in its constructor
}

PUBLIC_SYMBOL bool retro_load_game(const struct retro_game_info *info) {
    retro::log(RETRO_LOG_DEBUG, "retro_load_game(\"%s\", %d)\n", info->path, info->size);

    return melonds::load_games(info, nullptr);
}

PUBLIC_SYMBOL void retro_run(void) {
    using namespace melonds;
    using retro::log;
    using Config::Retro::CurrentRenderer;

    if (deferred_initialization_pending) {
        log(RETRO_LOG_DEBUG, "Starting deferred initialization");
        bool game_loaded = melonds::load_game_deferred(melonds::_nds_game_info, melonds::_gba_game_info);
        deferred_initialization_pending = false;
        if (!game_loaded) {
            // If we couldn't load the game...
            log(RETRO_LOG_ERROR, "Deferred initialization failed; exiting core");
            retro::environment(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
            return;
        }
        log(RETRO_LOG_DEBUG, "Completed deferred initialization");
    }

    if (!first_frame_run) {
        if (NdsSaveManager->SramLength() > 0) {
            NDS::LoadSave(NdsSaveManager->Sram(), NdsSaveManager->SramLength());
        }

        if (GbaSaveManager->SramLength() > 0) {
            GBACart::LoadSave(GbaSaveManager->Sram(), GbaSaveManager->SramLength());
        }

        // This has to be deferred even if we're not using OpenGL,
        // because libretro doesn't set the SRAM until after retro_load_game
        first_frame_run = true;
    }

    melonds::update_input(input_state);

    if (melonds::input_state.swap_screens_btn != Config::ScreenSwap) {
        switch (Config::Retro::ScreenSwapMode) {
            case melonds::ScreenSwapMode::Toggle: {
                if (!Config::ScreenSwap) {
                    swap_screen_toggled = !swap_screen_toggled;
                    update_screenlayout(current_screen_layout(), &screen_layout_data,
                                        CurrentRenderer == Renderer::OpenGl,
                                        swap_screen_toggled);
                    melonds::opengl::RequestOpenGlRefresh();
                }

                Config::ScreenSwap = input_state.swap_screens_btn;
                log(RETRO_LOG_DEBUG, "Toggled screen-swap mode (now %s)", Config::ScreenSwap ? "on" : "off");
                break;
            }
            case ScreenSwapMode::Hold: {
                if (Config::ScreenSwap != input_state.swap_screens_btn) {
                    log(RETRO_LOG_DEBUG, "%s holding the screen-swap button",
                        input_state.swap_screens_btn ? "Started" : "Stopped");
                }
                Config::ScreenSwap = input_state.swap_screens_btn;
                update_screenlayout(current_screen_layout(), &screen_layout_data,
                                    CurrentRenderer == Renderer::OpenGl,
                                    Config::ScreenSwap);
                melonds::opengl::RequestOpenGlRefresh();
            }
        }
    }

    auto mic_input_mode = static_cast<MicInputMode>(Config::MicInputType);

    if (Config::Retro::MicButtonRequired && !input_state.holding_noise_btn) {
        mic_input_mode = melonds::MicInputMode::None;
    }

    switch (mic_input_mode) {
        case MicInputMode::WhiteNoise: // random noise
        {
            s16 tmp[735];
            for (int i = 0; i < 735; i++) tmp[i] = rand() & 0xFFFF;
            NDS::MicInputFrame(tmp, 735);
            break;
        }
        case MicInputMode::BlowNoise: // blow noise
        {
            Frontend::Mic_FeedNoise(); // despite the name, this feeds a blow noise
            break;
        }
        case MicInputMode::HostMic: // microphone input
        {
            s16 tmp[735];
//                if (micHandle && micInterface.interface_version &&
//                    micInterface.get_mic_state(micHandle)) { // If the microphone is enabled and supported...
//                    micInterface.read_mic(micHandle, tmp, 735);
//                    NDS::MicInputFrame(tmp, 735);
//                    break;
//                } // If the mic isn't available, go to the default case
        }
        default:
            Frontend::Mic_FeedSilence();
    }

    if (melonds::render::ReadyToRender()) { // If the global state needed for rendering is ready...
        // NDS::RunFrame invokes rendering-related code
        NDS::RunFrame();

        // TODO: Use RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE
        melonds::render_frame();
        melonds::render_audio();
    }

    bool updated = false;
    if (retro::environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
        melonds::check_variables(false);

        struct retro_system_av_info updated_av_info{};
        retro_get_system_av_info(&updated_av_info);
        retro::environment(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &updated_av_info);
        screen_layout_data.clean_screenlayout_buffer();
    }
}

static void melonds::render_frame() {
    switch (Config::Retro::CurrentRenderer) {
#ifdef HAVE_OPENGL
        case Renderer::OpenGl:
            melonds::opengl::render_frame();
            break;
#endif
        case Renderer::Software:
        default:
            render::RenderSoftware();
            break;
    }
}

static void melonds::render_audio() {
    static int16_t audio_buffer[0x1000]; // 4096 samples == 2048 stereo frames
    u32 size = std::min(SPU::GetOutputSize(), static_cast<int>(sizeof(audio_buffer) / (2 * sizeof(int16_t))));
    // Ensure that we don't overrun the buffer

    size_t read = SPU::ReadOutput(audio_buffer, size);
    retro::audio_sample_batch(audio_buffer, read);
}

PUBLIC_SYMBOL void retro_unload_game(void) {
    retro::log(RETRO_LOG_DEBUG, "retro_unload_game()");
    // TODO: If this is homebrew, save the data
    // No need to flush SRAM, Platform::WriteNDSSave has been doing that for us this whole time
    NDS::Stop();
    NDS::DeInit();
    melonds::_loaded_nds_cart.reset();
    melonds::_loaded_gba_cart.reset();
}

PUBLIC_SYMBOL unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

PUBLIC_SYMBOL bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
    retro::log(RETRO_LOG_DEBUG, "retro_load_game_special(%s, %p, %u)", melonds::get_game_type_name(type), info, num);

    return melonds::load_games(&info[0], num > 1 ? &info[1] : nullptr);
}

PUBLIC_SYMBOL void retro_deinit(void) {
    retro::log(RETRO_LOG_DEBUG, "retro_deinit()");
    melonds::_base_directory.clear();
    melonds::_save_directory.clear();
    melonds::clear_memory_config();
    melonds::_loaded_nds_cart.reset();
    Platform::DeInit();
}

PUBLIC_SYMBOL unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

PUBLIC_SYMBOL void retro_get_system_info(struct retro_system_info *info) {
    info->library_name = "melonDS DS";
    info->block_extract = false;
    info->library_version = "0.0.0";
    info->need_fullpath = false;
    info->valid_extensions = "nds|ids|dsi";
}

PUBLIC_SYMBOL void retro_reset(void) {
    using melonds::_nds_game_info;
    retro::log(RETRO_LOG_DEBUG, "retro_reset()\n");
    NDS::Reset();

    melonds::first_frame_run = false;

    melonds::set_up_direct_boot(_nds_game_info);
}

static bool melonds::load_games(const struct retro_game_info *nds_info, const struct retro_game_info *gba_info) {
    melonds::clear_memory_config();
    melonds::check_variables(true);

    using retro::environment;
    using retro::log;
    using retro::set_message;

    /*
    * FIXME: Less bad than copying the whole data pointer, but still not great.
    * NDS::Reset() calls wipes the cart buffer so on invoke we need a reload from info->data.
    * Since retro_reset callback doesn't pass the info struct we need to cache it
    * here.
    */
    _nds_game_info = nds_info;

    retro_assert(_loaded_nds_cart == nullptr);
    retro_assert(_loaded_gba_cart == nullptr);

    // First parse the ROM...
    _loaded_nds_cart = std::make_unique<NDSCartData>(static_cast<const u8 *>(nds_info->data),
                                                     static_cast<u32>(nds_info->size));
    if (!_loaded_nds_cart->IsValid()) {
        _loaded_nds_cart.reset();
        retro::set_error_message("Failed to load the ROM. Is it a valid NDS ROM?");
        return false;
    }

    retro::log(RETRO_LOG_DEBUG, "Loaded NDS ROM: \"%s\"\n", nds_info->path);

    // Get the length of the ROM's SRAM, if any
    u32 sram_length = _loaded_nds_cart->Cart()->GetSaveMemoryLength();
    NdsSaveManager->SetSaveSize(sram_length);
    // Homebrew is a special case, as it uses a file system rather than SRAM.

    if (gba_info) {
        // If we want to load a GBA ROM...
        if (Config::ConsoleType == ConsoleType::DSi) {
            retro::set_warn_message("The DSi does not support GBA connectivity. Not loading the requested GBA ROM.");
        } else {
            _gba_game_info = gba_info;
            _loaded_gba_cart = std::make_unique<GBACartData>(static_cast<const u8 *>(gba_info->data),
                                                             static_cast<u32>(gba_info->size));
            if (!_loaded_gba_cart->IsValid()) {
                _loaded_gba_cart.reset();
                retro::set_error_message("Failed to load the GBA ROM. Is it a valid GBA ROM?");
                return false;
            }

            u32 gba_sram_length = _loaded_gba_cart->Cart()->GetSaveMemoryLength();
            GbaSaveManager->SetSaveSize(gba_sram_length);

            retro::log(RETRO_LOG_DEBUG, "Loaded GBA ROM: \"%s\"\n", gba_info->path);
        }
    }

    initialize_bios();

    // TODO: Ensure that the username is non-empty
    // TODO: Cap the username to match the DS's limit (10 chars, excluding null terminator)
    const char *retro_username;
    if (environment(RETRO_ENVIRONMENT_GET_USERNAME, &retro_username) && retro_username)
        Config::FirmwareUsername = retro_username;
    else
        Config::FirmwareUsername = "melonDS";

    environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void *) &melonds::input_descriptors);

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        log(RETRO_LOG_ERROR, "Failed to set XRGB8888, which is not supported.\n");
        return false;
    }

#ifdef HAVE_OPENGL
    // Initialize the opengl state if needed
    switch (Config::Retro::ConfiguredRenderer) {
        // Depending on which renderer we want to use...
        case Renderer::OpenGl:
            if (melonds::opengl::initialize()) {
                Config::Retro::CurrentRenderer = Renderer::OpenGl;
            } else {
                Config::Retro::CurrentRenderer = Renderer::Software;
                log(RETRO_LOG_ERROR, "Failed to initialize OpenGL renderer, falling back to software rendering");
                // TODO: Display a message stating that we're falling back to software rendering
            }
            break;
        default:
            log(RETRO_LOG_WARN, "Unknown renderer %d, falling back to software rendering",
                static_cast<int>(Config::Retro::ConfiguredRenderer));
            // Intentional fall-through
        case Renderer::Software:
            Config::Retro::CurrentRenderer = Renderer::Software;
            log(RETRO_LOG_INFO, "Using software renderer");
            break;
    }
#else
    log(RETRO_LOG_INFO, "OpenGL is not supported by this build, using software renderer");
#endif

    if (!NDS::Init()) {
        retro::set_error_message("Failed to initialize melonDS");
        return false;
    }

    SPU::SetInterpolation(Config::AudioInterp);
    NDS::SetConsoleType(Config::ConsoleType);

    if (Config::Retro::CurrentRenderer == Renderer::OpenGl) {
        log(RETRO_LOG_INFO, "Deferring initialization until the OpenGL context is ready");
        deferred_initialization_pending = true;
        return true;
    } else {
        log(RETRO_LOG_INFO, "No need to defer initialization, proceeding now");
        return load_game_deferred(nds_info, gba_info);
    }
}

static void melonds::initialize_bios() {
    using retro::log;

    if (Config::ExternalBIOSEnable) {
        // melonDS doesn't properly fall back to FreeBIOS if the external bioses are missing,
        // so we have to do it ourselves

        // TODO: Don't always check all files; just check for the ones we need
        // based on the console type
        std::array<std::string, 3> required_roms = {Config::BIOS7Path, Config::BIOS9Path, Config::FirmwarePath};
        std::vector<std::string> missing_roms;

        // Check if any of the bioses / firmware files are missing
        for (std::string &rom: required_roms) {
            if (Platform::LocalFileExists(rom)) {
                log(RETRO_LOG_INFO, "Found %s", rom.c_str());
            } else {
                missing_roms.push_back(rom);
                log(RETRO_LOG_WARN, "Could not find %s", rom.c_str());
            }
        }

        // TODO: Check both $SYSTEM/filename and $SYSTEM/melonDS DS/filename

        // Abort if there are any of the required roms are missing
        if (!missing_roms.empty()) {
            Config::ExternalBIOSEnable = false;
            retro::log(RETRO_LOG_WARN, "Using FreeBIOS instead of the aforementioned missing files.");
        }
    } else {
        retro::log(RETRO_LOG_INFO, "External BIOS is disabled, using FreeBIOS instead.");
    }
}

// melonDS tightly couples the renderer with the rest of the emulation code,
// so we can't initialize the emulator until the OpenGL context is ready.
static bool melonds::load_game_deferred(
    const struct retro_game_info *nds_info,
    const struct retro_game_info *gba_info
) {
    using retro::log;

    // GPU config must be initialized before NDS::Reset is called.
    // Ensure that there's a renderer, even if we're about to throw it out.
    // (GPU::SetRenderSettings may try to deinitialize a non-existing renderer)
    GPU::InitRenderer(Config::Retro::CurrentRenderer == Renderer::OpenGl);
    GPU::RenderSettings render_settings = Config::Retro::RenderSettings();
    GPU::SetRenderSettings(Config::Retro::CurrentRenderer == Renderer::OpenGl, render_settings);

    NDS::Reset(); // Loads the BIOS, too

    // The ROM and save data must be loaded after NDS::Reset is called

    retro_assert(_loaded_nds_cart != nullptr);
    retro_assert(_loaded_nds_cart->IsValid());

    bool inserted = NDSCart::InsertROM(std::move(*_loaded_nds_cart));
    _loaded_nds_cart.reset();
    if (!inserted) {
        // If we failed to insert the ROM, we can't continue
        retro::log(RETRO_LOG_ERROR, "Failed to insert \"%s\" into the emulator. Exiting.", nds_info->path);
        retro::set_error_message("Failed to insert the loaded ROM. Please report this issue.");
        return false;
    }

    if (gba_info && _loaded_gba_cart) {
        inserted = GBACart::InsertROM(std::move(*_loaded_gba_cart));
        _loaded_gba_cart.reset();
        if (!inserted) {
            // If we failed to insert the ROM, we can't continue
            retro::log(RETRO_LOG_ERROR, "Failed to insert \"%s\" into the emulator. Exiting.", gba_info->path);
            retro::set_error_message("Failed to insert the loaded ROM. Please report this issue.");
            return false;
        }
    }

    set_up_direct_boot(nds_info);

    NDS::Start();

    log(RETRO_LOG_INFO, "Initialized emulated console and loaded emulated game");

//    micInterface.interface_version = RETRO_MICROPHONE_INTERFACE_VERSION;
//    if (environ_cb(RETRO_ENVIRONMENT_GET_MICROPHONE_INTERFACE, &micInterface))
//    { // ...and if the current audio driver supports microphones...
//        if (micInterface.interface_version != RETRO_MICROPHONE_INTERFACE_VERSION)
//        {
//            log_cb(RETRO_LOG_WARN, "[melonDS] Expected mic interface version %u, got %u. Compatibility issues are possible.\n",
//                   RETRO_MICROPHONE_INTERFACE_VERSION, micInterface.interface_version);
//        }
//
//        log_cb(RETRO_LOG_DEBUG, "[melonDS] Microphone support available in current audio driver (version %u)\n",
//               micInterface.interface_version);
//
//        retro_microphone_params_t params = {
//                .rate = 44100 // The core engine assumes this rate
//        };
//        micHandle = micInterface.open_mic(&params);
//
//        if (micHandle)
//        {
//            log_cb(RETRO_LOG_INFO, "[melonDS] Initialized microphone\n");
//        }
//        else
//        {
//            log_cb(RETRO_LOG_WARN, "[melonDS] Failed to initialize microphone, emulated device will receive silence\n");
//        }
//    }

    return true;
}

static void melonds::set_up_direct_boot(const retro_game_info *nds_info) {
    if (Config::DirectBoot || NDS::NeedsDirectBoot()) {
        char game_name[256];
        const char *ptr = path_basename(nds_info->path);
        if (ptr)
            strlcpy(game_name, ptr, sizeof(game_name));
        else
            strlcpy(game_name, nds_info->path, sizeof(game_name));
        path_remove_extension(game_name);

        NDS::SetupDirectBoot(game_name);
    }
}
