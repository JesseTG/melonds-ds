#include <ctime>
#include <cstdlib>
#include <cstring>

#include <compat/strl.h>
#include <file/file_path.h>
#include <libretro.h>
#include <streams/file_stream.h>
#include <streams/file_stream_transforms.h>

#include <NDS.h>
#include <NDSCart.h>
#include <frontend/FrontendUtil.h>
#include <Platform.h>
#include <frontend/qt_sdl/Config.h>
#include <GPU.h>
#include <SPU.h>

#include "opengl.hpp"
#include "environment.hpp"
#include "config.hpp"
#include "libretro.hpp"
#include "input.hpp"
#include "utils.hpp"
#include "info.hpp"
#include "screenlayout.hpp"

namespace melonds {
    static std::string base_directory;
    static std::string save_directory;
    static const retro_game_info *game_info;
    static bool swapped_screens = false;

    static void render_frame();

    static void render_audio();

    static bool load_game(unsigned type, const struct retro_game_info *info);
}


PUBLIC_SYMBOL void retro_init(void) {
    const char *dir = nullptr;

    srand(time(nullptr));
    if (retro::environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
        melonds::base_directory = dir;

    if (retro::environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
        melonds::save_directory = dir;

    // ScreenLayoutData is initialized in its constructor
}

PUBLIC_SYMBOL bool retro_load_game(const struct retro_game_info *info) {
    return melonds::load_game(0, info);
}

PUBLIC_SYMBOL void retro_run(void) {
    using melonds::input_state;
    melonds::update_input(input_state);

    if (melonds::input_state.swap_screens_btn != melonds::swapped_screens) {
        if (toggle_swap_screen) {
            if (!melonds::swapped_screens) {
                swap_screen_toggled = !swap_screen_toggled;
                update_screenlayout(current_screen_layout, &screen_layout_data, enable_opengl, swap_screen_toggled);
                refresh_opengl = true;
            }

            melonds::swapped_screens = input_state.swap_screens_btn;
        } else {
            melonds::swapped_screens = input_state.swap_screens_btn;
            update_screenlayout(current_screen_layout, &screen_layout_data, enable_opengl, melonds::swapped_screens);
            refresh_opengl = true;
        }
    }

    if (input_state.holding_noise_btn || !noise_button_required) {
        switch (micNoiseType) {
            case WhiteNoise: // random noise
            {
                s16 tmp[735];
                for (int i = 0; i < 735; i++) tmp[i] = rand() & 0xFFFF;
                NDS::MicInputFrame(tmp, 735);
                break;
            }
            case BlowNoise: // blow noise
            {
                Frontend::Mic_FeedNoise(); // despite the name, this feeds a blow noise
                break;
            }
//            case MicInput: // microphone input
//            {
//                s16 tmp[735];
//                if (micHandle && micInterface.interface_version &&
//                    micInterface.get_mic_state(micHandle)) { // If the microphone is enabled and supported...
//                    micInterface.read_mic(micHandle, tmp, 735);
//                    NDS::MicInputFrame(tmp, 735);
//                    break;
//                } // If the mic isn't available, go to the default case
//            }
            default:
                Frontend::Mic_FeedSilence();
        }
    } else {
        Frontend::Mic_FeedSilence();
    }

    if (current_renderer != melonds::CurrentRenderer::None) NDS::RunFrame();

    melonds::render_frame();

    melonds::render_audio();

    bool updated = false;
    if (retro::environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
        melonds::check_variables(false);

        struct retro_system_av_info updated_av_info{};
        retro_get_system_av_info(&updated_av_info);
        retro::environment(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &updated_av_info);
        clean_screenlayout_buffer(&screen_layout_data);
    }

    NDSCart_SRAMManager::Flush();
}

static void melonds::render_frame() {
    using melonds::screen_layout_data;
    if (current_renderer == CurrentRenderer::None) {
#ifdef HAVE_OPENGL
        if (Config::ScreenUseGL && melonds::opengl::using_opengl()) {
            // Try to initialize opengl, if it failed fallback to software
            if (melonds::opengl::initialize()) {
                current_renderer = CurrentRenderer::OpenGLRenderer;
            } else {
                return;
            }
        } else {
            if (melonds::opengl::using_opengl()) melonds::opengl::deinitialize();

            current_renderer = CurrentRenderer::Software;

        }
#else
        current_renderer = CurrentRenderer::Software;
#endif
    }
#ifdef HAVE_OPENGL
    if (melonds::opengl::using_opengl()) {
        melonds::opengl::render_frame(current_renderer == CurrentRenderer::Software);
    } else if (!Config::ScreenUseGL) {
#endif
        int frontbuf = GPU::FrontBuffer;

        if (screen_layout_data.hybrid) {
            unsigned primary = screen_layout_data.displayed_layout == ScreenLayout::HybridTop ? 0 : 1;

            copy_hybrid_screen(&screen_layout_data, GPU::Framebuffer[frontbuf][primary], ScreenId::Primary);

            switch (screen_layout_data.hybrid_small_screen) {
                case SmallScreenLayout::SmallScreenTop:
                    copy_hybrid_screen(&screen_layout_data, GPU::Framebuffer[frontbuf][0], ScreenId::Bottom);
                    break;
                case SmallScreenLayout::SmallScreenBottom:
                    copy_hybrid_screen(&screen_layout_data, GPU::Framebuffer[frontbuf][1], ScreenId::Bottom);
                    break;
                case SmallScreenLayout::SmallScreenDuplicate:
                    copy_hybrid_screen(&screen_layout_data, GPU::Framebuffer[frontbuf][0], ScreenId::Top);
                    copy_hybrid_screen(&screen_layout_data, GPU::Framebuffer[frontbuf][1], ScreenId::Bottom);
                    break;
            }

            if (input_state.cursor_enabled())
                draw_cursor(&screen_layout_data, input_state.touch_x, input_state.touch_y);

            retro::video_refresh((uint8_t *) screen_layout_data.buffer_ptr, screen_layout_data.buffer_width,
                                 screen_layout_data.buffer_height, screen_layout_data.buffer_width * sizeof(uint32_t));
        } else {
            if (screen_layout_data.enable_top_screen)
                copy_screen(&screen_layout_data, GPU::Framebuffer[frontbuf][0], screen_layout_data.top_screen_offset);
            if (screen_layout_data.enable_bottom_screen)
                copy_screen(&screen_layout_data, GPU::Framebuffer[frontbuf][1],
                            screen_layout_data.bottom_screen_offset);

            if (input_state.cursor_enabled() && current_screen_layout() != ScreenLayout::TopOnly)
                draw_cursor(&screen_layout_data, input_state.touch_x, input_state.touch_y);

            retro::video_refresh((uint8_t *) screen_layout_data.buffer_ptr, screen_layout_data.buffer_width,
                                 screen_layout_data.buffer_height, screen_layout_data.buffer_width * sizeof(uint32_t));
        }
#ifdef HAVE_OPENGL
    }
#endif
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
    // TODO: Does this clear the underlying memory?
    melonds::base_directory.clear();
    melonds::save_directory.clear();
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
    NDS::LoadCart((u8 *) game_info->data, game_info->size, _save_path.c_str(), Config::DirectBoot);
    // TODO: Load game
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
    if (Config::_3DRenderer == melonds::RendererType::OpenGl)
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

    Config::SaveFilePath = save_directory + PLATFORM_DIR_SEPERATOR + std::string(game_name) + ".sav";

    GPU::InitRenderer(false);
    GPU::SetRenderSettings(false, melonds::render_settings());
    SPU::SetInterpolation(Config::AudioInterp);
    NDS::SetConsoleType(Config::ConsoleType);
    NDS::LoadBIOS();

    if (!NDSCart::LoadROM((const uint8_t *) info->data, info->size)) {
        retro::log(RETRO_LOG_ERROR, "Failed to load ROM");
    }

    if (Platform::FileExists(Config::SaveFilePath)) {
        void *save_data = nullptr;
        int64_t save_length = 0;

        if (filestream_read_file(Config::SaveFilePath.c_str(), &save_data, &save_length)) {
            retro::log(RETRO_LOG_INFO, "Loaded save file from %s", Config::SaveFilePath.c_str());

            NDS::LoadSave(static_cast<const uint8_t *>(save_data), save_length);
            free(save_data);
        }
    }

    if (type == melonds::SLOT_1_2_BOOT) {
        char gba_game_name[256];
        std::string gba_save_path;
        const char *gba_ptr = path_basename(info[1].path);
        if (gba_ptr)
            strlcpy(gba_game_name, gba_ptr, sizeof(gba_game_name));
        else
            strlcpy(gba_game_name, info[1].path, sizeof(gba_game_name));
        path_remove_extension(gba_game_name);

        gba_save_path = melonds::save_directory + PLATFORM_DIR_SEPERATOR + std::string(gba_game_name) + ".srm";

        if (!NDS::LoadGBACart((const uint8_t *) info[1]->data, info[1]->size)) {
            retro::log(RETRO_LOG_ERROR, "Failed to load ROM");
        }

        if (Platform::FileExists(Config::SaveFilePath)) {
            void *save_data = nullptr;
            int64_t save_length = 0;

            if (filestream_read_file(Config::SaveFilePath.c_str(), &save_data, &save_length)) {
                retro::log(RETRO_LOG_INFO, "Loaded save file from %s", Config::SaveFilePath.c_str());

                NDS::LoadSave(static_cast<const uint8_t *>(save_data), save_length);
                free(save_data);
            }
        }
    }

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
