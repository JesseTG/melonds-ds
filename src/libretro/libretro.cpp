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

#include "opengl.hpp"
#include "environment.hpp"
#include "config.hpp"
#include "input.hpp"
#include "utils.hpp"
#include "info.hpp"
#include "screenlayout.hpp"
#include "memory.hpp"

namespace melonds {
    static std::string _base_directory;
    static std::string _save_directory;
    static const retro_game_info *game_info;
    static bool swap_screen_toggled = false;

    static void render_frame();

    static void render_audio();

    static bool load_game(unsigned type, const struct retro_game_info *info);

    static void render_software();
}

const std::string &retro::base_directory() {
    return melonds::_base_directory;
}

const std::string &retro::save_directory() {
    return melonds::_save_directory;
}

PUBLIC_SYMBOL void retro_init(void) {
    const char *dir = nullptr;

    srand(time(nullptr));
    if (retro::environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
        melonds::_base_directory = dir;

    if (retro::environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
        melonds::_save_directory = dir;

    Platform::Init(0, nullptr);
    melonds::init_savestate_buffer();
    // ScreenLayoutData is initialized in its constructor
}

PUBLIC_SYMBOL bool retro_load_game(const struct retro_game_info *info) {
    return melonds::load_game(0, info);
}

PUBLIC_SYMBOL void retro_run(void) {
    using namespace melonds;

    melonds::update_input(input_state);

    if (melonds::input_state.swap_screens_btn != Config::ScreenSwap) {
        switch (Config::Retro::ScreenSwapMode) {
            case melonds::ScreenSwapMode::Toggle: {
                if (!Config::ScreenSwap) {
                    swap_screen_toggled = !swap_screen_toggled;
                    update_screenlayout(current_screen_layout(), &screen_layout_data, Config::ScreenUseGL,
                                        swap_screen_toggled);
                    melonds::opengl::refresh_opengl = true;
                }

                Config::ScreenSwap = input_state.swap_screens_btn;
                break;
            }
            case ScreenSwapMode::Hold: {
                Config::ScreenSwap = input_state.swap_screens_btn;
                update_screenlayout(current_screen_layout(), &screen_layout_data, Config::ScreenUseGL,
                                    Config::ScreenSwap);
                melonds::opengl::refresh_opengl = true;
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

    if (Config::Retro::CurrentRenderer != melonds::CurrentRenderer::None) NDS::RunFrame();

    melonds::render_frame();

    melonds::render_audio();

    bool updated = false;
    if (retro::environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
        melonds::check_variables(false);

        struct retro_system_av_info updated_av_info{};
        retro_get_system_av_info(&updated_av_info);
        retro::environment(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &updated_av_info);
        screen_layout_data.clean_screenlayout_buffer();
    }

    //NDSCart_SRAMManager::Flush();
    // TODO: Flush SRAM to disk, if necessary
}

static void melonds::render_frame() {
    using melonds::screen_layout_data;
#ifdef HAVE_OPENGL
    if (Config::Retro::CurrentRenderer == CurrentRenderer::None) {
        // If we haven't initialized a renderer  yet...
        if (Config::ScreenUseGL && Config::Retro::UsingOpenGl) {
            // Try to initialize opengl, if it failed fallback to software
            if (melonds::opengl::initialize()) {
                Config::Retro::CurrentRenderer = CurrentRenderer::OpenGl;
            } else {
                Config::Retro::UsingOpenGl = false;
                return;
            }
        } else {
            if (Config::Retro::UsingOpenGl) {
                melonds::opengl::deinitialize();
            }

            Config::Retro::CurrentRenderer = CurrentRenderer::Software;
        }
    }

    if (Config::Retro::UsingOpenGl) {
        melonds::opengl::render_frame(Config::Retro::CurrentRenderer == CurrentRenderer::Software);
    } else if (!Config::ScreenUseGL) {
        render_software();
    }
#else
    if (Config::Retro::CurrentRenderer == CurrentRenderer::None) {
        Config::Retro::CurrentRenderer = CurrentRenderer::Software;
    }

    render_software();
#endif
}

static void melonds::render_software() {
    int frontbuf = GPU::FrontBuffer;

    if (screen_layout_data.hybrid) {
        unsigned primary = screen_layout_data.displayed_layout == ScreenLayout::HybridTop ? 0 : 1;

        screen_layout_data.copy_hybrid_screen(GPU::Framebuffer[frontbuf][primary], ScreenId::Primary);

        switch (screen_layout_data.hybrid_small_screen) {
            case SmallScreenLayout::SmallScreenTop:
                screen_layout_data.copy_hybrid_screen(GPU::Framebuffer[frontbuf][0], ScreenId::Bottom);
                break;
            case SmallScreenLayout::SmallScreenBottom:
                screen_layout_data.copy_hybrid_screen(GPU::Framebuffer[frontbuf][1], ScreenId::Bottom);
                break;
            case SmallScreenLayout::SmallScreenDuplicate:
                screen_layout_data.copy_hybrid_screen(GPU::Framebuffer[frontbuf][0], ScreenId::Top);
                screen_layout_data.copy_hybrid_screen(GPU::Framebuffer[frontbuf][1], ScreenId::Bottom);
                break;
        }

        if (input_state.cursor_enabled())
            screen_layout_data.draw_cursor(input_state.touch_x, input_state.touch_y);

        retro::video_refresh((uint8_t *) screen_layout_data.buffer_ptr, screen_layout_data.buffer_width,
                             screen_layout_data.buffer_height, screen_layout_data.buffer_width * sizeof(uint32_t));
    } else {
        if (screen_layout_data.enable_top_screen)
            screen_layout_data.copy_screen(GPU::Framebuffer[frontbuf][0], screen_layout_data.top_screen_offset);
        if (screen_layout_data.enable_bottom_screen)
            screen_layout_data.copy_screen(GPU::Framebuffer[frontbuf][1],
                                           screen_layout_data.bottom_screen_offset);

        if (input_state.cursor_enabled() && current_screen_layout() != ScreenLayout::TopOnly)
            screen_layout_data.draw_cursor(input_state.touch_x, input_state.touch_y);

        retro::video_refresh((uint8_t *) screen_layout_data.buffer_ptr, screen_layout_data.buffer_width,
                             screen_layout_data.buffer_height, screen_layout_data.buffer_width * sizeof(uint32_t));
    }
}

static void melonds::render_audio() {
    static int16_t audio_buffer[0x1000]; // 4096 samples == 2048 stereo frames
    u32 size = std::min(SPU::GetOutputSize(), static_cast<int>(sizeof(audio_buffer) / (2 * sizeof(int16_t))));
    // Ensure that we don't overrun the buffer

    SPU::ReadOutput(audio_buffer, size);
    retro::audio_sample_batch(audio_buffer, size);
}

PUBLIC_SYMBOL void retro_unload_game(void) {
    NDS::DeInit();
}

PUBLIC_SYMBOL unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

PUBLIC_SYMBOL bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
    return melonds::load_game(type, info);
}

PUBLIC_SYMBOL void retro_deinit(void) {
    melonds::_base_directory.clear();
    melonds::_save_directory.clear();
    melonds::free_savestate_buffer();
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
    info->valid_extensions = "nds|dsi";
}

PUBLIC_SYMBOL void retro_set_controller_port_device(unsigned port, unsigned device) {
    retro::log(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

PUBLIC_SYMBOL void retro_reset(void) {
    using melonds::game_info;
    NDS::Reset();
    if (!NDSCart::LoadROM((const uint8_t *) game_info->data, game_info->size)) {
        retro::log(RETRO_LOG_ERROR, "Failed to load ROM");
    }

    if (Platform::FileExists(Config::SaveFilePath)) {
        void *save_data = nullptr;
        int64_t save_length = 0;

        if (filestream_read_file(Config::SaveFilePath.c_str(), &save_data, &save_length)) {
            retro::log(RETRO_LOG_INFO, "Loaded %d-byte save file from %s", save_length, Config::SaveFilePath.c_str());

            NDS::LoadSave(static_cast<const uint8_t *>(save_data), save_length);
            free(save_data);
        }
    }
}

static bool melonds::load_game(unsigned type, const struct retro_game_info *info) {
    using retro::environment;
    using retro::log;

    /*
    * FIXME: Less bad than copying the whole data pointer, but still not great.
    * NDS::Reset() calls wipes the cart buffer so on invoke we need a reload from info->data.
    * Since retro_reset callback doesn't pass the info struct we need to cache it
    * here.
    */
    game_info = info;

    std::vector<std::string> required_roms = {"bios7.bin", "bios9.bin", "firmware.bin"};
    std::vector<std::string> missing_roms;

    // Check if any of the bioses / firmware files are missing
    for (std::string &rom: required_roms) {
        if (!Platform::LocalFileExists(rom)) {
            missing_roms.push_back(rom);
        }
    }

    // Abort if there are any of the required roms are missing
    if (!missing_roms.empty()) {
        std::string msg = "Using FreeBIOS instead of the following missing BIOS/firmware files: ";

        for (int i = 0; i < missing_roms.size(); ++i) {
            const std::string &missing_rom = missing_roms[i];

            msg += missing_rom;
            if (i < missing_roms.size() - 1) {
                msg += ", ";
            }
        }

        msg.append("\n");

        retro::log(RETRO_LOG_ERROR, msg.c_str());
    }

    Config::BIOS7Path = "bios7.bin";
    Config::BIOS9Path = "bios9.bin";
    Config::FirmwarePath = "firmware.bin";
    Config::DSiBIOS7Path = "dsi_bios7.bin";
    Config::DSiBIOS9Path = "dsi_bios9.bin";
    Config::DSiFirmwarePath = "dsi_firmware.bin";
    Config::DSiNANDPath = "dsi_nand.bin";
    Config::DSiSDPath = "dsi_sd_card.bin";

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

    melonds::check_variables(true);

    // Initialize the opengl state if needed
#ifdef HAVE_OPENGL
    if (Config::ScreenUseGL)
        melonds::opengl::initialize();
#endif

    if (!NDS::Init())
        return false;

    char game_name[256];
    const char *ptr = path_basename(info->path);
    if (ptr)
        strlcpy(game_name, ptr, sizeof(game_name));
    else
        strlcpy(game_name, info->path, sizeof(game_name));
    path_remove_extension(game_name);

    Config::SaveFilePath = _save_directory + PLATFORM_DIR_SEPERATOR + std::string(game_name) + ".sav";

    // GPU config must be initialized before NDS::Reset is called
    GPU::InitRenderer(false);
    GPU::RenderSettings render_settings = Config::Retro::RenderSettings();
    GPU::SetRenderSettings(false, render_settings);
    SPU::SetInterpolation(Config::AudioInterp);
    NDS::SetConsoleType(Config::ConsoleType);

    NDS::Reset(); // Loads the BIOS, too

    // The ROM and save data must be loaded after NDS::Reset is called

    if (!NDSCart::LoadROM((const uint8_t *) info->data, info->size)) {
        retro::log(RETRO_LOG_ERROR, "Failed to load ROM");
    }

    if (Platform::FileExists(Config::SaveFilePath)) {
        void *save_data = nullptr;
        int64_t save_length = 0;

        if (filestream_read_file(Config::SaveFilePath.c_str(), &save_data, &save_length)) {
            retro::log(RETRO_LOG_INFO, "Loaded %d-byte save file from %s\n", save_length, Config::SaveFilePath.c_str());

            NDS::LoadSave(static_cast<const uint8_t *>(save_data), save_length);
            free(save_data);
        }
    }

    if (type == melonds::SLOT_1_2_BOOT) {
        char gba_game_name[256];
        const char *gba_ptr = path_basename(info[1].path);
        if (gba_ptr)
            strlcpy(gba_game_name, gba_ptr, sizeof(gba_game_name));
        else
            strlcpy(gba_game_name, info[1].path, sizeof(gba_game_name));
        path_remove_extension(gba_game_name);

        std::string gba_save_path =
                melonds::_save_directory + PLATFORM_DIR_SEPERATOR + std::string(gba_game_name) + ".srm";
        if (!GBACart::LoadROM((const uint8_t *) info[1].data, info[1].size)) {
            retro::log(RETRO_LOG_ERROR, "Failed to load GBA ROM");
        }

        if (Platform::FileExists(gba_save_path)) {
            void *gba_save_data = nullptr;
            int64_t gba_save_length = 0;

            if (filestream_read_file(Config::SaveFilePath.c_str(), &gba_save_data, &gba_save_length)) {
                retro::log(RETRO_LOG_INFO, "Loaded GBA save file from %s", Config::SaveFilePath.c_str());

                GBACart::LoadSave(static_cast<const u8 *>(gba_save_data), gba_save_length);
                free(gba_save_data);
            }
        }
    }

    if (Config::DirectBoot || NDS::NeedsDirectBoot()) {
        NDS::SetupDirectBoot(game_name);
    }

    NDS::Start();

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
