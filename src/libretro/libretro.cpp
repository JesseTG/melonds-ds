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
#include <memory>

#include <compat/strl.h>
#include <file/file_path.h>
#include <libretro.h>
#include <streams/rzip_stream.h>
#include <streams/file_stream.h>

#include <DSi.h>
#include <NDS.h>
#include <NDSCart.h>
#include <frontend/FrontendUtil.h>
#include <Platform.h>
#include <GPU.h>
#include <SPU.h>
#include <GBACart.h>
#include <retro_assert.h>
#include <retro_miscellaneous.h>
#include <string/stdstring.h>

#include "opengl.hpp"
#include "content.hpp"
#include "environment.hpp"
#include "config.hpp"
#include "input.hpp"
#include "utils.hpp"
#include "info.hpp"
#include "screenlayout.hpp"
#include "memory.hpp"
#include "render.hpp"
#include "exceptions.hpp"
#include "microphone.hpp"
#include "dsi.hpp"

using std::make_optional;
using std::optional;
using std::nullopt;

namespace melonds {
    static bool swap_screen_toggled = false;
    static bool deferred_initialization_pending = false;
    static bool first_frame_run = false;
    static std::unique_ptr<NdsCart> _loaded_nds_cart;
    static std::unique_ptr<GbaCart> _loaded_gba_cart;
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
    static void parse_nds_rom(const struct retro_game_info &info);
    static void init_nds_save(const NdsCart &nds_cart);
    static void parse_gba_rom(const struct retro_game_info &info);
    static void init_gba_save(GbaCart &gba_cart, const struct retro_game_info& gba_save_info);
    static void init_rendering();
    static void load_games_deferred(
        const optional<retro_game_info>& nds_info,
        const optional<retro_game_info>& gba_info
    );
    static void set_up_direct_boot(const retro_game_info &nds_info);
    static void install_sram(
        const optional<retro_game_info>& nds_info,
        const optional<retro_game_info>& gba_info
    );
    static void ValidateFirmware();

    // functions for running games
    static void read_microphone(melonds::InputState& input_state) noexcept;
    static void render_audio();
    static void flush_save_data() noexcept;
    static void flush_gba_sram(const retro_game_info& gba_save_info) noexcept;
}

PUBLIC_SYMBOL void retro_init(void) {
    retro::log(RETRO_LOG_DEBUG, "retro_init");
    retro_assert(melonds::_loaded_nds_cart == nullptr);
    retro_assert(melonds::_loaded_gba_cart == nullptr);
    retro_assert(retro::content::get_loaded_nds_info() == nullopt);
    retro_assert(retro::content::get_loaded_gba_info() == nullopt);
    retro_assert(retro::content::get_loaded_gba_save_info() == nullopt);
    srand(time(nullptr));

    melonds::first_frame_run = false;
    // ScreenLayoutData is initialized in its constructor
}

static bool melonds::handle_load_game(unsigned type, const struct retro_game_info *info, size_t num) noexcept try {
    retro_assert(_loaded_nds_cart == nullptr);
    retro_assert(_loaded_gba_cart == nullptr);
    retro_assert(retro::content::get_loaded_nds_info() == nullopt);
    retro_assert(retro::content::get_loaded_gba_info() == nullopt);
    retro_assert(retro::content::get_loaded_gba_save_info() == nullopt);

    // First initialize the content info...
    switch (type) {
        case melonds::MELONDSDS_GAME_TYPE_NDS:
            // ...which refers to a Nintendo DS game...
            retro::content::set_loaded_content_info(info, nullptr);
            break;
        case melonds::MELONDSDS_GAME_TYPE_SLOT_1_2_BOOT:
            // ...which refers to both a Nintendo DS and Game Boy Advance game...
            switch (num) {
                case 2: // NDS ROM and GBA ROM
                    retro::content::set_loaded_content_info(info, info + 1);
                    break;
                case 3: // NDS ROM, GBA ROM, and GBA SRAM
                    retro::content::set_loaded_content_info(info, info + 1, info + 2);
                    break;
                default:
                    retro::log(RETRO_LOG_ERROR, "Invalid number of ROMs (%u) for slot-1/2 boot", num);
                    retro::set_error_message(melonds::INTERNAL_ERROR_MESSAGE);
            }
            break;
        default:
            retro::log(RETRO_LOG_ERROR, "Unknown game type %d", type);
            retro::set_error_message(melonds::INTERNAL_ERROR_MESSAGE);
            return false;
    }

    // ...then load the game.
    melonds::load_games(
        retro::content::get_loaded_nds_info(),
        retro::content::get_loaded_gba_info(),
        retro::content::get_loaded_gba_save_info()
    );

    return true;
}
catch (const melonds::emulator_exception &e) {
    // Thrown for invalid ROMs
    retro::error("%s", e.what());
    retro::set_error_message(e.user_message());
    _loaded_nds_cart.reset();
    _loaded_gba_cart.reset();
    return false;
}
catch (const std::exception &e) {
    retro::log(RETRO_LOG_ERROR, "%s", e.what());
    retro::set_error_message(melonds::INTERNAL_ERROR_MESSAGE);
    _loaded_nds_cart.reset();
    _loaded_gba_cart.reset();
    return false;
}
catch (...) {
    retro::set_error_message(melonds::UNKNOWN_ERROR_MESSAGE);
    _loaded_nds_cart.reset();
    _loaded_gba_cart.reset();
    return false;
}

PUBLIC_SYMBOL bool retro_load_game(const struct retro_game_info *info) {
    if (info) {
        retro::debug("retro_load_game(\"%s\", %d)", info->path, info->size);
    }
    else {
        retro::debug("retro_load_game(<no content>)");
    }

    return melonds::handle_load_game(melonds::MELONDSDS_GAME_TYPE_NDS, info, 1);
}

static void melonds::install_sram(
    const optional<retro_game_info>& nds_info,
    const optional<retro_game_info>& gba_info
) {
    // Nintendo DS SRAM is loaded by the frontend
    // and copied into NdsSaveManager via the pointer returned by retro_get_memory.
    // This is where we install the SRAM data into the emulated DS.
    if (nds_info && melonds::NdsSaveManager->SramLength() > 0) {
        // If we're loading a NDS game that has SRAM...
        NDS::LoadSave(melonds::NdsSaveManager->Sram(), melonds::NdsSaveManager->SramLength());
    }

    // GBA SRAM is selected by the user explicitly (due to libretro limits) and loaded by the frontend,
    // but is not processed by retro_get_memory (again due to libretro limits).
    if (gba_info && melonds::GbaSaveManager->SramLength() > 0) {
        // If we're loading a GBA game that has existing SRAM...
        // TODO: Decide what to do about SRAM files that append extra metadata like the RTC
        GBACart::LoadSave(melonds::GbaSaveManager->Sram(), melonds::GbaSaveManager->SramLength());
    }

    // We could've installed the GBA's SRAM in retro_load_game (since it's not processed by retro_get_memory),
    // but doing so here helps keep things tidier since the NDS SRAM is installed here too.
}

static void melonds::flush_save_data() noexcept {
    if (TimeToGbaFlush != std::nullopt) {
        if (*TimeToGbaFlush > 0) { // std::optional::operator> checks the optional's validity for us
            // If we have a GBA SRAM flush coming up...
            *TimeToGbaFlush -= 1;
        }

        if (*TimeToGbaFlush <= 0) {
            // If it's time to flush the GBA's SRAM...
            const optional<retro_game_info>& gba_save_info = retro::content::get_loaded_gba_save_info();
            if (gba_save_info) {
                // If we actually have GBA save data loaded...
                flush_gba_sram(*gba_save_info);
            }
            TimeToGbaFlush = std::nullopt; // Reset the timer
        }
    }
}

static void melonds::flush_gba_sram(const retro_game_info& gba_save_info) noexcept {

    const char* save_data_path = gba_save_info.path;
    if (save_data_path == nullptr || GbaSaveManager == nullptr) {
        // No save data path was provided, or the GBA save manager isn't initialized
        return; // TODO: Report this error
    }
    const u8* gba_sram = GbaSaveManager->Sram();
    u32 gba_sram_length = GbaSaveManager->SramLength();

    if (gba_sram == nullptr || gba_sram_length == 0) {
        return; // TODO: Report this error
    }

    if (!filestream_write_file(save_data_path, gba_sram, gba_sram_length)) {
        retro::error("Failed to write %u-byte GBA SRAM to \"%s\"", gba_sram_length, save_data_path);
        // TODO: Report this to the user
    }
    else {
        retro::debug("Flushed %u-byte GBA SRAM to \"%s\"", gba_sram_length, save_data_path);
    }
}

PUBLIC_SYMBOL void retro_run(void) {
    using namespace melonds;
    using retro::log;

    if (deferred_initialization_pending) {
        try {
            log(RETRO_LOG_DEBUG, "Starting deferred initialization");
            melonds::load_games_deferred(
                retro::content::get_loaded_nds_info(),
                retro::content::get_loaded_gba_info()
            );
            deferred_initialization_pending = false;

            log(RETRO_LOG_DEBUG, "Completed deferred initialization");
        }
        catch (const melonds::emulator_exception& e) {
            log(RETRO_LOG_ERROR, "Deferred initialization failed; exiting core");
            retro::error(e.what());
            retro::set_error_message(e.user_message());
            retro::shutdown();
            return;
        }
        catch (const std::exception& e) {
            log(RETRO_LOG_ERROR, "Deferred initialization failed; exiting core");
            retro::set_error_message(e.what());
            retro::shutdown();
            return;
        }
        catch (...) {
            log(RETRO_LOG_ERROR, "Deferred initialization failed; exiting core");
            retro::set_error_message(UNKNOWN_ERROR_MESSAGE);
            retro::shutdown();
            return;
        }
    }

    if (!first_frame_run) {
        // Apply the save data from the core's SRAM buffer to the cart's SRAM;
        // we need to do this in the first frame of retro_run because
        // retro_get_memory_data is used to copy the loaded SRAM
        // in between retro_load and the first retro_run call.
        install_sram(
            retro::content::get_loaded_nds_info(),
            retro::content::get_loaded_gba_info()
        );

        // This has to be deferred even if we're not using OpenGL,
        // because libretro doesn't set the SRAM until after retro_load_game
        first_frame_run = true;
    }

    melonds::update_input(input_state);

    if (melonds::input_state.swap_screens_btn != screen_layout_data.SwapScreens()) {
        switch (config::screen::ScreenSwapMode()) {
            case melonds::ScreenSwapMode::Toggle: {
                if (!screen_layout_data.SwapScreens()) {
                    swap_screen_toggled = !swap_screen_toggled;
                    screen_layout_data.SwapScreens(swap_screen_toggled);
                    screen_layout_data.Update(render::CurrentRenderer());
                    melonds::opengl::RequestOpenGlRefresh();
                }

                screen_layout_data.SwapScreens(input_state.swap_screens_btn);
                log(RETRO_LOG_DEBUG, "Toggled screen-swap mode (now %s)", screen_layout_data.SwapScreens() ? "on" : "off");
                break;
            }
            case ScreenSwapMode::Hold: {
                if (screen_layout_data.SwapScreens() != input_state.swap_screens_btn) {
                    log(RETRO_LOG_DEBUG, "%s holding the screen-swap button",
                        input_state.swap_screens_btn ? "Started" : "Stopped");
                }

                screen_layout_data.SwapScreens(input_state.swap_screens_btn);
                screen_layout_data.Update(render::CurrentRenderer());
                melonds::opengl::RequestOpenGlRefresh();
            }
        }
    }

    if (melonds::render::ReadyToRender()) { // If the global state needed for rendering is ready...
        read_microphone(input_state);

        // NDS::RunFrame invokes rendering-related code
        NDS::RunFrame();

        render::Render(input_state);
        melonds::render_audio();
        melonds::flush_save_data();
    }

    if (bool updated = false; retro::environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated) {
        optional<NDSHeader> header = NDSCart::Cart ? std::make_optional(NDSCart::Cart->GetHeader()) : std::nullopt;
        melonds::UpdateConfig(retro::content::get_loaded_nds_info(), header);

        struct retro_system_av_info updated_av_info{};
        retro_get_system_av_info(&updated_av_info);
        retro::environment(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &updated_av_info);
        screen_layout_data.clean_screenlayout_buffer();
        // TODO: Replace with RETRO_ENVIRONMENT_SET_GEOMETRY (requires fixing hybrid mode)
    }
}

static void melonds::read_microphone(melonds::InputState& input_state) noexcept {
    MicInputMode mic_input_mode = config::audio::MicInputMode();

    switch (config::audio::MicButtonMode()) {
        // If the microphone button...
        case MicButtonMode::Hold: {
            // ...must be held...
            if (!input_state.holding_noise_btn) {
                // ...but it isn't...
                mic_input_mode = MicInputMode::None;
            }
            break;
        }
        case MicButtonMode::Always: {
            // ...is unnecessary...
            // Do nothing, the mic input mode is already set
        }
    }

    if (retro::microphone::is_open()) {
        retro::microphone::set_state(input_state.holding_noise_btn || config::audio::MicButtonMode() == MicButtonMode::Always);
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
            // TODO: Consider switching to Mic_FeedExternalBuffer
            const auto mic_state = retro::microphone::get_state();
            if (mic_state.has_value() && mic_state.value()) {
                // If the microphone is open and turned on...
                s16 samples[735];
                retro::microphone::read(samples, ARRAY_SIZE(samples));
                NDS::MicInputFrame(samples, ARRAY_SIZE(samples));
                break;
            }
            // If the mic isn't available, feed silence instead
        }
        // Intentional fall-through
        default:
            Frontend::Mic_FeedSilence();
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
    // No need to flush SRAM to the buffer, Platform::WriteNDSSave has been doing that for us this whole time
    // No need to flush the homebrew save data either, the CartHomebrew destructor does that
    const optional<struct retro_game_info>& gba_save_info = retro::content::get_loaded_gba_save_info();
    if (gba_save_info) {
        melonds::flush_gba_sram(*gba_save_info);
    }
    NDS::Stop();
    NDS::DeInit();

    const optional<struct retro_game_info>& nds_info = retro::content::get_loaded_nds_info();
    if (nds_info && melonds::_loaded_nds_cart && melonds::_loaded_nds_cart->GetHeader().IsDSiWare()) {
        melonds::dsi::uninstall_dsiware(*nds_info, *melonds::_loaded_nds_cart);
    }

    melonds::_loaded_nds_cart.reset();
    melonds::_loaded_gba_cart.reset();
}

PUBLIC_SYMBOL unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

PUBLIC_SYMBOL bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
    retro::log(RETRO_LOG_DEBUG, "retro_load_game_special(%s, %p, %u)", melonds::get_game_type_name(type), info, num);

    return melonds::handle_load_game(type, info, num);
}

PUBLIC_SYMBOL void retro_deinit(void) {
    retro::log(RETRO_LOG_DEBUG, "retro_deinit()");
    retro::clear_environment();
    retro::content::clear();
    melonds::clear_memory_config();
    melonds::_loaded_nds_cart.reset();
    melonds::_loaded_gba_cart.reset();
    Platform::DeInit();
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

PUBLIC_SYMBOL void retro_reset(void) {
    retro::log(RETRO_LOG_DEBUG, "retro_reset()\n");
    NDS::Reset();

    melonds::first_frame_run = false;

    const auto &nds_info = retro::content::get_loaded_nds_info();
    if (nds_info && melonds::_loaded_nds_cart && !melonds::_loaded_nds_cart->GetHeader().IsDSiWare()) {
        melonds::set_up_direct_boot(nds_info.value());
    }
}

static void melonds::parse_nds_rom(const struct retro_game_info &info) {
    _loaded_nds_cart = NDSCart::ParseROM(static_cast<const u8 *>(info.data), static_cast<u32>(info.size));

    if (!_loaded_nds_cart) {
        throw invalid_rom_exception("Failed to parse the DS ROM image. Is it valid?");
    }

    retro::log(RETRO_LOG_DEBUG, "Loaded NDS ROM: \"%s\"", info.path);
}

static void melonds::parse_gba_rom(const struct retro_game_info &info) {
    _loaded_gba_cart = GBACart::ParseROM(static_cast<const u8 *>(info.data), static_cast<u32>(info.size));

    if (!_loaded_gba_cart) {
        throw invalid_rom_exception("Failed to parse the GBA ROM image. Is it valid?");
    }

    retro::log(RETRO_LOG_DEBUG, "Loaded GBA ROM: \"%s\"", info.path);
}

// Loads the GBA SRAM
static void melonds::init_gba_save(GbaCart& gba_cart, const struct retro_game_info& gba_save_info) {
    // We load the GBA SRAM file ourselves (rather than letting the frontend do it)
    // because we'll overwrite it later and don't want the frontend to hold open any file handles.

    if (path_contains_compressed_file(gba_save_info.path)) {
        // If this save file is in an archive (e.g. /path/to/file.7z#mygame.srm)...

        // We don't support GBA SRAM files in archives right now;
        // libretro-common has APIs for extracting and re-inserting them,
        // but I just can't be bothered.
        retro::set_error_message(
            "melonDS DS does not support archived GBA save data right now. "
            "Please extract it and try again. "
            "Continuing without using the save data."
        );

        return;
    }

    // rzipstream opens the file as-is if it's not rzip-formatted
    rzipstream_t* gba_save_file = rzipstream_open(gba_save_info.path, RETRO_VFS_FILE_ACCESS_READ);
    if (!gba_save_file) {
        throw std::runtime_error("Failed to open GBA save file");
    }

    if (rzipstream_is_compressed(gba_save_file)) {
        // If this save data is compressed in libretro's rzip format...
        // (not to be confused with a standard archive format like zip or 7z)

        // We don't support rzip-compressed GBA save files right now;
        // I can't be bothered.
        retro::set_error_message(
            "melonDS DS does not support compressed GBA save data right now. "
            "Please disable save data compression in the frontend and try again. "
            "Continuing without using the save data."
        );

        rzipstream_close(gba_save_file);
        return;
    }

    int64_t gba_save_file_size = rzipstream_get_size(gba_save_file);
    if (gba_save_file_size < 0) {
        // If we couldn't get the uncompressed size of the GBA save file...
        rzipstream_close(gba_save_file);
        throw std::runtime_error("Failed to get GBA save file size");
    }

    void* gba_save_data = malloc(gba_save_file_size);
    if (!gba_save_data) {
        rzipstream_close(gba_save_file);
        throw std::runtime_error("Failed to allocate memory for GBA save file");
    }

    if (rzipstream_read(gba_save_file, gba_save_data, gba_save_file_size) != gba_save_file_size) {
        rzipstream_close(gba_save_file);
        free(gba_save_data);
        throw std::runtime_error("Failed to read GBA save file");
    }

    melonds::GbaSaveManager->SetSaveSize(gba_save_file_size);
    gba_cart.SetupSave(gba_save_file_size);
    gba_cart.LoadSave(static_cast<const u8*>(gba_save_data), gba_save_file_size);
    retro::debug("Allocated %u-byte GBA SRAM", gba_cart.GetSaveMemoryLength());
    // Actually installing the SRAM will be done later, after NDS::Reset is called
    free(gba_save_data);
    rzipstream_close(gba_save_file);
}

static void melonds::load_games(
    const optional<struct retro_game_info> &nds_info,
    const optional<struct retro_game_info> &gba_info,
    const optional<struct retro_game_info> &gba_save_info
) {
    melonds::clear_memory_config();
    NDSHeader header;
    if (nds_info) {
        // Need to get the header before parsing the ROM,
        // as parsing the ROM can depend on the config
        // but the config can depend on the header.
        memcpy(&header, nds_info->data, sizeof(header));
    }
    melonds::InitConfig(nds_info, nds_info ? make_optional(header) : nullopt);

    Platform::Init(0, nullptr);

    using retro::environment;
    using retro::log;
    using retro::set_message;

    retro_assert(_loaded_nds_cart == nullptr);
    retro_assert(_loaded_gba_cart == nullptr);

    // First parse the ROMs...
    if (nds_info) {
        // NDS::Reset() calls wipes the cart buffer so on invoke we need a reload from info->data.
        // Since retro_reset callback doesn't pass the info struct we need to cache it.
        parse_nds_rom(*nds_info);

        // sanity check; parse_nds_rom does the real validation
        retro_assert(_loaded_nds_cart != nullptr);

        if (!_loaded_nds_cart->GetHeader().IsDSiWare()) {
            // If this ROM represents a cartridge, rather than DSiWare...
            init_nds_save(*_loaded_nds_cart);
        }
    }

    if (gba_info) {
        if (config::system::ConsoleType() == ConsoleType::DSi) {
            retro::set_warn_message("The DSi does not support GBA connectivity. Not loading the requested GBA ROM or SRAM.");
        } else {
            parse_gba_rom(*gba_info);

            if (gba_save_info) {
                init_gba_save(*_loaded_gba_cart, *gba_save_info);
            }
            else {
                retro::info("No GBA SRAM was provided.");
            }
        }
    }

    if (!config::system::ExternalBiosEnable() && _loaded_gba_cart) {
        // If we're using FreeBIOS and are trying to load a GBA cart...
        retro::set_warn_message(
            "FreeBIOS does not support GBA connectivity. "
            "Please install a native BIOS and enable it in the options menu."
        );
    }

    environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void *) &melonds::input_descriptors);

    init_rendering();

    if (!NDS::Init()) {
        retro::log(RETRO_LOG_ERROR, "Failed to initialize melonDS DS.");
        throw std::runtime_error("Failed to initialize NDS emulator.");
    }

    SPU::SetInterpolation(static_cast<int>(config::audio::Interpolation()));
    NDS::SetConsoleType(static_cast<int>(config::system::ConsoleType()));

    if (render::CurrentRenderer() == Renderer::OpenGl) {
        log(RETRO_LOG_INFO, "Deferring initialization until the OpenGL context is ready");
        deferred_initialization_pending = true;
    } else {
        log(RETRO_LOG_INFO, "No need to defer initialization, proceeding now");
        load_games_deferred(nds_info, gba_info);
    }
}

// Does not load the NDS SRAM, since retro_get_memory is used for that.
// But it will allocate the SRAM buffer
static void melonds::init_nds_save(const NdsCart &nds_cart) {
    using std::runtime_error;
     if (nds_cart.GetHeader().IsHomebrew()) {
         // If this is a homebrew ROM...

         // Homebrew is a special case, as it uses an SD card rather than SRAM.
         // No need to explicitly load or save homebrew SD card images;
         // the CartHomebrew class does that.
         if (config::save::DldiFolderSync()) {
             // If we're syncing the homebrew SD card image to the host filesystem...
             if (!path_mkdir(config::save::DldiFolderPath().c_str())) {
                 // Create the directory. If that fails...
                 // (note that an existing directory is not an error)
                 throw runtime_error("Failed to create virtual SD card directory at " + config::save::DldiFolderPath());
             }
         }
     }
     else {
        // Get the length of the ROM's SRAM, if any
        u32 sram_length = nds_cart.GetSaveMemoryLength();
        NdsSaveManager->SetSaveSize(sram_length);

        if (sram_length > 0) {
            retro::log(RETRO_LOG_DEBUG, "Allocated %u-byte SRAM buffer for loaded NDS ROM.", sram_length);
        } else {
            retro::log(RETRO_LOG_DEBUG, "Loaded NDS ROM does not use SRAM.");
        }
        // The actual SRAM file is installed later; it's loaded into the core via retro_get_memory_data,
        // and it's applied in the first frame of retro_run.
    }
}

// melonDS tightly couples the renderer with the rest of the emulation code,
// so we can't initialize the emulator until the OpenGL context is ready.
static void melonds::load_games_deferred(
    const optional<retro_game_info>& nds_info,
    const optional<retro_game_info>& gba_info
) {
    using retro::log;

    // GPU config must be initialized before NDS::Reset is called.
    // Ensure that there's a renderer, even if we're about to throw it out.
    // (GPU::SetRenderSettings may try to deinitialize a non-existing renderer)
    bool isOpenGl = render::CurrentRenderer() == Renderer::OpenGl;
    GPU::InitRenderer(isOpenGl);
    GPU::RenderSettings render_settings = config::video::RenderSettings();
    GPU::SetRenderSettings(isOpenGl, render_settings);

    // Loads the BIOS, too
    NDS::Reset();

    ValidateFirmware();

    // The ROM must be inserted after NDS::Reset is called

    retro_assert(NDSCart::Cart == nullptr);

    if (_loaded_nds_cart) {
        // If we want to insert a NDS ROM that was previously loaded...

        if (!_loaded_nds_cart->GetHeader().IsDSiWare()) {
            // If we're running a physical cartridge...

            if (!NDSCart::InsertROM(std::move(_loaded_nds_cart))) {
                // If we failed to insert the ROM, we can't continue
                retro::log(RETRO_LOG_ERROR, "Failed to insert \"%s\" into the emulator. Exiting.", nds_info->path);
                throw std::runtime_error("Failed to insert the loaded ROM. Please report this issue.");
            }

            // Just to be sure
            retro_assert(_loaded_nds_cart == nullptr);
            retro_assert(NDSCart::Cart != nullptr);
        }
        else {
            // We're running a DSiWare game, then
            if (!DSi::LoadNAND()) {
                throw std::runtime_error("Failed to load NAND. Please report this issue.");
            }
            melonds::dsi::install_dsiware(*nds_info, *_loaded_nds_cart);
        }
    }

    retro_assert(GBACart::Cart == nullptr);

    if (gba_info && _loaded_gba_cart) {
        // If we want to insert a GBA ROM that was previously loaded...
        bool inserted = GBACart::InsertROM(std::move(_loaded_gba_cart));
        if (!inserted) {
            // If we failed to insert the ROM, we can't continue
            retro::log(RETRO_LOG_ERROR, "Failed to insert \"%s\" into the emulator. Exiting.", gba_info->path);
            throw std::runtime_error("Failed to insert the loaded ROM. Please report this issue.");
        }

        retro_assert(_loaded_gba_cart == nullptr);
    }

    if (nds_info && NDSCart::Cart && !NDSCart::Cart->GetHeader().IsDSiWare()) {
        set_up_direct_boot(*nds_info);
    }

    NDS::Start();

    log(RETRO_LOG_INFO, "Initialized emulated console and loaded emulated game");
}

static void melonds::init_rendering() {
    using retro::environment;
    using retro::log;

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        throw std::runtime_error("Failed to set the required XRGB8888 pixel format for rendering; it may not be supported.");
    }

    melonds::render::Initialize(config::video::ConfiguredRenderer());

}

// Decrypts the ROM's secure area
static void melonds::set_up_direct_boot(const retro_game_info &nds_info) {
    if (config::system::DirectBoot() || NDS::NeedsDirectBoot()) {
        char game_name[256];
        const char *ptr = path_basename(nds_info.path);
        if (ptr)
            strlcpy(game_name, ptr, sizeof(game_name));
        else
            strlcpy(game_name, nds_info.path, sizeof(game_name));

        NDS::SetupDirectBoot(game_name);
        retro::log(RETRO_LOG_DEBUG, "Initialized direct boot for \"%s\"", game_name);
    }
}

static void melonds::ValidateFirmware() {
    using namespace config::system;
    if (!ExternalBiosEnable() || config::system::ConsoleType() != ConsoleType::DS) {
        // If we're not using an external BIOS, or we're not emulating a DS...
        return;
    }

    // I don't really know how this works, it just came from upstream
    // TODO: Peek at the firmware buffer in SPI directly (need to send a PR to expose it)
    FILE* f = Platform::OpenLocalFile(config::system::FirmwarePath(), "rb");
    if (!f) return;
    u8 chk1[0x180], chk2[0x180];

    fseek(f, 0, SEEK_SET);
    fread(chk1, 1, 0x180, f);
    fseek(f, -0x380, SEEK_END);
    fread(chk2, 1, 0x180, f);

    memset(&chk1[0x0C], 0, 8);
    memset(&chk2[0x0C], 0, 8);

    fclose(f);

    if (!memcmp(chk1, chk2, 0x180))
    {
        retro::set_warn_message("You're using a hacked firmware dump.\n"
            "Firmware boot will stop working if you run any game that alters WFC settings.\n\n"
            "(This would also happen on real hardware.)");
    }
}