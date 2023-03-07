#include <ctime>
#include <cstdlib>
#include <cstring>
#include <libretro.h>
#include <NDS.h>
#include <NDSCart.h>
#include <Platform.h>
#include <frontend/qt_sdl/Config.h>
#include <GPU.h>
#include <SPU.h>

#include "environment.hpp"
#include "config.hpp"
#include "libretro.hpp"
#include "input.hpp"

namespace retro {

    static bool load_game(unsigned type, const struct retro_game_info *info);
    static std::string _base_directory;
    static std::string _save_directory;
    static std::string _save_path;
    static const retro_game_info* _game_info;
}


PUBLIC_SYMBOL void retro_init(void) {
    const char *dir = nullptr;

    srand(time(nullptr));
    if (retro::environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
        retro::_base_directory = dir;

    if (retro::environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
        retro::_save_directory = dir;
}

PUBLIC_SYMBOL bool retro_load_game(const struct retro_game_info *info) {
    return retro::load_game(0, info);
}

PUBLIC_SYMBOL void retro_unload_game(void) {
    NDS::DeInit();
}

PUBLIC_SYMBOL unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

PUBLIC_SYMBOL bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
    return retro::load_game(type, info);
}

PUBLIC_SYMBOL void retro_deinit(void) {
    // TODO: Does this clear the underlying memory?
    retro::_base_directory.clear();
    retro::_save_directory.clear();
    retro::_save_path.clear();
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
    using retro::_game_info;
    using retro::_save_path;
    NDS::Reset();
    NDS::LoadROM((u8*)_game_info->data, _game_info->size, _save_path.c_str(), Config::DirectBoot);
    // TODO: Load game
}

const std::string &retro::base_directory() {
    return _base_directory;
}

const std::string &retro::save_directory() {
    return _save_directory;
}

static bool retro::load_game(unsigned type, const struct retro_game_info *info)
{
    /*
    * FIXME: Less bad than copying the whole data pointer, but still not great.
    * NDS::Reset() calls wipes the cart buffer so on invoke we need a reload from info->data.
    * Since retro_reset callback doesn't pass the info struct we need to cache it
    * here.
    */
    _game_info = info;

    std::vector <std::string> required_roms = {"bios7.bin", "bios9.bin", "firmware.bin"};
    std::vector <std::string> missing_roms;

    // Check if any of the bioses / firmware files are missing
    for(std::string& rom : required_roms)
    {
        if(!Platform::LocalFileExists(rom.c_str()))
        {
            missing_roms.push_back(rom);
        }
    }

    // Abort if there are any of the required roms are missing
    if(!missing_roms.empty())
    {
        std::string msg = "Missing bios/firmware in system directory. Using FreeBIOS.";

        int i = 0;
        int len = missing_roms.size();
        for (auto missing_rom : missing_roms)
        {
            msg.append(missing_rom);
            if(len - 1 > i) msg.append(", ");
            i ++;
        }

        msg.append("\n");

        log(RETRO_LOG_ERROR, msg.c_str());
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


    environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)&melonds::input_descriptors);

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    {
        log(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
        return false;
    }

    melonds::check_variables(true);

    // Initialize the opengl state if needed
#ifdef HAVE_OPENGL
    if (_renderer_type == melonds::RendererType::OpenGl)
        initialize_opengl();
#endif

    if(!NDS::Init())
        return false;

    char game_name[256];
    const char *ptr = path_basename(info->path);
    if (ptr)
        strlcpy(game_name, ptr, sizeof(game_name));
    else
        strlcpy(game_name, info->path, sizeof(game_name));
    path_remove_extension(game_name);

    save_path = std::string(retro_saves_directory) + std::string(1, PLATFORM_DIR_SEPERATOR) + std::string(game_name) + ".sav";

    GPU::InitRenderer(false);
    GPU::SetRenderSettings(false, video_settings);
    SPU::SetInterpolation(Config::AudioInterp);
    NDS::SetConsoleType(Config::ConsoleType);
    Frontend::LoadBIOS();
    NDS::LoadROM((u8*)info->data, info->size, save_path.c_str(), Config::DirectBoot);

    if (type == SLOT_1_2_BOOT)
    {
        char gba_game_name[256];
        std::string gba_save_path;
        const char *ptr = path_basename(info[1].path);
        if (ptr)
            strlcpy(gba_game_name, ptr, sizeof(gba_game_name));
        else
            strlcpy(gba_game_name, info[1].path, sizeof(gba_game_name));
        path_remove_extension(gba_game_name);

        gba_save_path = std::string(retro_saves_directory) + std::string(1, PLATFORM_DIR_SEPERATOR) + std::string(gba_game_name) + ".srm";

        NDS::LoadGBAROM((u8*)info[1].data, info[1].size, gba_game_name, gba_save_path.c_str());
    }

    micInterface.interface_version = RETRO_MICROPHONE_INTERFACE_VERSION;
    if (environ_cb(RETRO_ENVIRONMENT_GET_MICROPHONE_INTERFACE, &micInterface))
    { // ...and if the current audio driver supports microphones...
        if (micInterface.interface_version != RETRO_MICROPHONE_INTERFACE_VERSION)
        {
            log_cb(RETRO_LOG_WARN, "[melonDS] Expected mic interface version %u, got %u. Compatibility issues are possible.\n",
                   RETRO_MICROPHONE_INTERFACE_VERSION, micInterface.interface_version);
        }

        log_cb(RETRO_LOG_DEBUG, "[melonDS] Microphone support available in current audio driver (version %u)\n",
               micInterface.interface_version);

        retro_microphone_params_t params = {
                .rate = 44100 // The core engine assumes this rate
        };
        micHandle = micInterface.open_mic(&params);

        if (micHandle)
        {
            log_cb(RETRO_LOG_INFO, "[melonDS] Initialized microphone\n");
        }
        else
        {
            log_cb(RETRO_LOG_WARN, "[melonDS] Failed to initialize microphone, emulated device will receive silence\n");
        }
    }

    return true;
}