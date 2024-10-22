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

#include "console.hpp"

#include <codecvt>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

#include <FreeBIOS.h>
#include <NDS.h>
#include <DSi.h>

#include <encodings/utf.h>
#include <file/file_path.h>
#include <retro_assert.h>
#include <retro_timers.h>
#include <streams/file_stream.h>
#include <streams/rzip_stream.h>
#include <string/stdstring.h>

#include "config.hpp"
#include "environment.hpp"
#include "exceptions.hpp"
#include "format.hpp"
#include "retro/file.hpp"
#include "retro/http.hpp"
#include "retro/info.hpp"
#include "types.hpp"

using std::make_optional;
using std::make_unique;
using std::nullopt;
using std::optional;
using std::span;
using std::u16string;
using std::string_view;
using std::unique_ptr;
using melonDS::Firmware;
using melonDS::NDSHeader;
using namespace melonDS::DSi_NAND;
using melonDS::DSi_TMD::TitleMetadata;

namespace MelonDsDs {
    const char *TMD_DIR_NAME = "tmd";
    const char* SENTINEL_NAME = "melon.dat";
    constexpr uint32_t RSA256_SIGNATURE_TYPE = 16777472;

    static melonDS::NDSArgs GetNdsArgs(
        const CoreConfig& config,
        const retro::GameInfo* ndsInfo,
        const retro::GameInfo* gbaInfo,
        const retro::GameInfo* gbaSaveInfo,
        CoreState& state
    );
    static melonDS::DSiArgs GetDSiArgs(const CoreConfig& config, const retro::GameInfo* ndsInfo);
    static void ApplyCommonArgs(const CoreConfig& config, melonDS::NDSArgs& args) noexcept;
    static unique_ptr<melonDS::NDSCart::CartCommon> LoadNdsCart(const CoreConfig& config, const retro::GameInfo& ndsInfo);
    static unique_ptr<melonDS::GBACart::CartCommon> LoadGbaCart(const retro::GameInfo& gbaInfo, const retro::GameInfo* gbaSaveInfo);
    static std::pair<unique_ptr<uint8_t[]>, size_t> LoadGbaSram(const retro::GameInfo& gbaSaveInfo);
    static void InstallDsiware(NANDMount& mount, const retro::GameInfo& nds_info);
    static void GetTmdPath(const retro::GameInfo &nds_info, std::span<char> buffer);
    static optional<TitleMetadata> GetCachedTmd(string_view tmdPath) noexcept;
    static bool ValidateTmd(const TitleMetadata &tmd) noexcept;
    static optional<TitleMetadata> DownloadTmd(const NDSHeader& header, string_view tmdPath) noexcept;
    static bool CacheTmd(string_view tmd_path, std::span<const std::byte> tmd) noexcept;
    static void ImportDsiwareSaveData(NANDMount& nand, const retro::GameInfo& nds_info, const NDSHeader& header, int type) noexcept;
    static optional<Firmware> LoadFirmware(const string& firmwarePath) noexcept;
    static bool LoadBios(const string_view& name, BiosType type, std::span<uint8_t> buffer) noexcept;
    static void CustomizeFirmware(const CoreConfig& config, Firmware& firmware);
    static std::optional<std::string> GetUsername(UsernameMode mode) noexcept;
    static NANDImage LoadNANDImage(const string& nandPath, const uint8_t* es_keyY);
    static void CustomizeNAND(const CoreConfig& config, NANDMount& mount, const NDSHeader* header, string_view nandName);
    static optional<melonDS::FATStorage> LoadDSiSDCardImage(const CoreConfig& config) noexcept;
    static std::optional<std::u16string> ConvertUsername(string_view str) noexcept;

    static constexpr Firmware::Language GetFirmwareLanguage(retro_language language) noexcept {
        switch (language) {
            case RETRO_LANGUAGE_ENGLISH:
            case RETRO_LANGUAGE_BRITISH_ENGLISH:
                return Firmware::Language::English;
            case RETRO_LANGUAGE_JAPANESE:
                return Firmware::Language::Japanese;
            case RETRO_LANGUAGE_FRENCH:
                return Firmware::Language::French;
            case RETRO_LANGUAGE_GERMAN:
                return Firmware::Language::German;
            case RETRO_LANGUAGE_ITALIAN:
                return Firmware::Language::Italian;
            case RETRO_LANGUAGE_SPANISH:
                return Firmware::Language::Spanish;
            case RETRO_LANGUAGE_CHINESE_SIMPLIFIED:
            case RETRO_LANGUAGE_CHINESE_TRADITIONAL:
                // The DS/DSi itself doesn't seem to distinguish between the two variants;
                // different regions just have one or the other.
                return Firmware::Language::Chinese;
            default:
                return Firmware::Language::English;
        }
    }
}

std::unique_ptr<melonDS::NDS> MelonDsDs::CreateConsole(
    CoreState& state,
    const CoreConfig& config,
    const retro::GameInfo* ndsInfo,
    const retro::GameInfo* gbaInfo,
    const retro::GameInfo* gbaSaveInfo
) {
    ZoneScopedN(TracyFunction);
    ConsoleType type = config.ConsoleType();
    const melonDS::NDSHeader* header = ndsInfo
        ? reinterpret_cast<const melonDS::NDSHeader*>(ndsInfo->GetData().data())
        : nullptr;

    if (header && header->IsDSiWare()) {
        // If we're loading a DSiWare game...
        type = ConsoleType::DSi;
        retro::warn("Forcing DSi mode for DSiWare game");
    }

    if (type == ConsoleType::DSi) {
        // If we're in DSi mode...
        if (gbaInfo || gbaSaveInfo) {
            retro::set_warn_message(
                "The DSi does not support GBA connectivity. Not loading the requested GBA ROM or SRAM."
            );
        }
        return std::make_unique<melonDS::DSi>(GetDSiArgs(config, ndsInfo), &state);
    }
    else {
        // If we're in DS mode...
        return std::make_unique<melonDS::NDS>(GetNdsArgs(config, ndsInfo, gbaInfo, gbaSaveInfo, state), &state);
    }
}

void MelonDsDs::UpdateConsole(const CoreConfig& config, melonDS::NDS& nds) noexcept {
    ZoneScopedN(TracyFunction);

    nds.SPU.SetInterpolation(config.Interpolation());
    nds.SPU.SetDegrade10Bit(config.BitDepth());
}

// First, load the system files
// Then, validate the system files
// Then, fall back to other system files if needed and possible
// If fallback is needed and not possible, throw an exception
// Finally, install the system files
static melonDS::NDSArgs MelonDsDs::GetNdsArgs(
    const CoreConfig& config,
    const retro::GameInfo* ndsInfo,
    const retro::GameInfo* gbaInfo,
    const retro::GameInfo* gbaSaveInfo,
    CoreState& state
) {
    ZoneScopedN(TracyFunction);

    // The rules are somewhat complicated.
    // - Bootable firmware is required if booting without content.
    // - All system files must be native or all must be built-in. (No mixing.)
    // - If BIOS files are built-in, then Direct Boot mode must be used
    optional<Firmware> firmware;
    if (config.SysfileMode() == SysfileMode::Native) {
        optional<string> firmwarePath = retro::get_system_path(config.FirmwarePath());
        if (!firmwarePath) {
            retro::error("Failed to get system directory");
        }

        firmware = firmwarePath ? LoadFirmware(*firmwarePath) : nullopt;
    }

    if (!ndsInfo && !(firmware && firmware->IsBootable())) {
        // If we're trying to boot into the NDS menu, but we didn't load bootable firmware...
        if (config.SysfileMode() == SysfileMode::Native) {
            throw nds_firmware_not_bootable_exception(config.FirmwarePath());
        }
        else {
            throw nds_firmware_not_bootable_exception();
        }
    }

    if (!firmware) {
        // If we haven't loaded any firmware...
        if (config.SysfileMode() == SysfileMode::Native) {
            // ...but we were trying to...
            retro::warn("Falling back to built-in firmware");
        }
        firmware = make_optional<Firmware>(static_cast<int>(ConsoleType::DS));
    }

    bool isFirmwareGenerated = firmware->GetHeader().Identifier == melonDS::GENERATED_FIRMWARE_IDENTIFIER;
    if (isFirmwareGenerated) {
        retro::debug("Not loading native ARM BIOS files");
    }

    melonDS::NDSArgs ndsargs {};

    ApplyCommonArgs(config, ndsargs);

    retro_assert(ndsargs.ARM7BIOS != nullptr);
    retro_assert(ndsargs.ARM9BIOS != nullptr);

    // Try to load the ARM7 and ARM9 BIOS files (but don't bother with the ARM9 BIOS if the ARM7 BIOS failed)
    bool bios7Loaded = !isFirmwareGenerated && LoadBios(config.Bios7Path(), BiosType::Arm7, *ndsargs.ARM7BIOS);
    bool bios9Loaded = bios7Loaded && LoadBios(config.Bios9Path(), BiosType::Arm9, *ndsargs.ARM9BIOS);

    if (config.SysfileMode() == SysfileMode::Native && !(bios7Loaded && bios9Loaded)) {
        // If we're trying to load native BIOS files, but at least one of them failed...
        retro::warn("Falling back to FreeBIOS");
    }

    // Now that we've loaded the system files, let's see if we can use them

    auto bootMode = config.BootMode();
    if (bootMode == BootMode::Native && !(bios7Loaded && bios9Loaded && firmware->IsBootable())) {
        // If we want to try a native boot, but the BIOS files aren't all native or the firmware isn't bootable...
        retro::warn("Native boot requires bootable firmware and native BIOS files; forcing Direct Boot mode");

        bootMode = BootMode::Direct;
        // TODO: Check whether this is necessary later, when actually booting the DS
    }

    if (!ndsInfo && !(firmware && firmware->IsBootable() && bios7Loaded && bios9Loaded)) {
        // If we're trying to boot into the NDS menu, but we don't have all the required files...
        throw nds_sysfiles_incomplete_exception();
    }

    if (bios7Loaded && bios9Loaded) {
        retro::debug("Installed native ARM7 and ARM9 NDS BIOS images");
    }
    else {
        ndsargs.ARM9BIOS = std::make_unique<melonDS::ARM9BIOSImage>(melonDS::bios_arm9_bin);
        ndsargs.ARM7BIOS = std::make_unique<melonDS::ARM7BIOSImage>(melonDS::bios_arm7_bin);
        retro::debug("Installed built-in ARM7 and ARM9 NDS BIOS images");
    }

    CustomizeFirmware(config, *firmware);
    ndsargs.Firmware = std::move(*firmware);

    if (ndsInfo) {
        ndsargs.NDSROM = LoadNdsCart(config, *ndsInfo);
        const uint8_t* romdata = ndsargs.NDSROM->GetROM();
        const NDSHeader &header = ndsargs.NDSROM->GetHeader();

        bool romDecrypted = (*(uint32_t*)&romdata[header.ARM9ROMOffset] == 0xE7FFDEFF && *(uint32_t*)&romdata[header.ARM9ROMOffset + 0x10] != 0xE7FFDEFF);
        if (!header.IsHomebrew() && !romDecrypted && !(bios7Loaded && bios9Loaded)) {
            // If this is an encrypted retail ROM but we aren't using the native BIOS...
            throw encrypted_rom_exception();
        }
    }

    if (gbaInfo) {
        // If loading a specific GBA ROM, then ignore the expansion paks
        ndsargs.GBAROM = LoadGbaCart(*gbaInfo, gbaSaveInfo);
    } else {
        switch (config.Slot2Device()) {
            case Slot2Device::MemoryExpansionPak:
                ndsargs.GBAROM = std::make_unique<melonDS::GBACart::CartRAMExpansion>();
                retro::debug("Installed built-in GBA Memory Expansion Pak");
                break;
            case Slot2Device::RumblePak:
                ndsargs.GBAROM = std::make_unique<melonDS::GBACart::CartRumblePak>(&state);
                retro::debug("Installed built-in GBA Rumble Pak");
                break;
            default:
                break;
        }
    }

    return ndsargs;
}

static melonDS::DSiArgs MelonDsDs::GetDSiArgs(const CoreConfig& config, const retro::GameInfo* ndsInfo) {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::system;
    using namespace MelonDsDs::config::firmware;

    string_view nandName = config.DsiNandPath();
    if (nandName == config::values::NOT_FOUND) {
        throw dsi_no_nand_found_exception();
    }

    if (config.DsiFirmwarePath() == config::values::NOT_FOUND) {
        throw dsi_no_firmware_found_exception();
    }

    // DSi mode requires all native BIOS files
    unique_ptr<melonDS::DSiBIOSImage> arm7i = make_unique<melonDS::DSiBIOSImage>();
    if (!LoadBios(config.DsiBios7Path(), BiosType::Arm7i, *arm7i)) {
        throw dsi_missing_bios_exception(BiosType::Arm7i, config.DsiBios7Path());
    }

    unique_ptr<melonDS::DSiBIOSImage> arm9i = make_unique<melonDS::DSiBIOSImage>();
    if (!LoadBios(config.DsiBios9Path(), BiosType::Arm9i, *arm9i)) {
        throw dsi_missing_bios_exception(BiosType::Arm9i, config.DsiBios9Path());
    }

    unique_ptr<melonDS::ARM7BIOSImage> arm7 = make_unique<melonDS::ARM7BIOSImage>();
    if (!LoadBios(config.Bios7Path(), BiosType::Arm7, *arm7)) {
        throw dsi_missing_bios_exception(BiosType::Arm7, config.Bios7Path());
    }

    unique_ptr<melonDS::ARM9BIOSImage> arm9 = make_unique<melonDS::ARM9BIOSImage>();
    if (!LoadBios(config.Bios9Path(), BiosType::Arm9, *arm9)) {
        throw dsi_missing_bios_exception(BiosType::Arm9, config.Bios9Path());
    }

    optional<string> firmwarePath = retro::get_system_path(config.DsiFirmwarePath());
    retro_assert(firmwarePath.has_value());
    // If we couldn't get the system directory, we wouldn't have gotten this far

    optional<Firmware> firmware = LoadFirmware(*firmwarePath);
    if (!firmware) {
        throw firmware_missing_exception(config.DsiFirmwarePath());
    }

    if (firmware->GetHeader().ConsoleType != Firmware::FirmwareConsoleType::DSi) {
        retro::warn("Expected firmware of type DSi, got {}", firmware->GetHeader().ConsoleType);
        throw wrong_firmware_type_exception(config.DsiFirmwarePath(), ConsoleType::DSi,
                                            firmware->GetHeader().ConsoleType);
    }
    // DSi firmware isn't bootable, so we don't need to check for that here.

    retro::debug("Installed native ARM7, ARM9, DSi ARM7, and DSi ARM9 BIOS images.");

    // TODO: Customize the NAND first, then use the final value of TWLCFG to patch the firmware
    CustomizeFirmware(config, *firmware);

    optional<string> nandPath = retro::get_system_path(nandName);
    if (!nandPath) {
        throw environment_exception("Failed to get the system directory, which means the NAND image can't be loaded.");
    }

    NANDImage nand = LoadNANDImage(*nandPath, &(*arm7i)[0x8308]);
    unique_ptr<melonDS::NDSCart::CartCommon> ndsRom = ndsInfo ? LoadNdsCart(config, *ndsInfo) : nullptr;

    { // Scoped to limit the mount's lifetime
        NANDMount mount(nand);
        if (!mount) {
            throw dsi_nand_corrupted_exception(nandName);
        }
        retro::debug("Opened and mounted the DSi NAND image file at {}", *nandPath);

        const NDSHeader* header = ndsInfo ? reinterpret_cast<const NDSHeader*>(ndsInfo->GetData().data()) : nullptr;
        CustomizeNAND(config, mount, header, nandName);

        if (ndsInfo && ndsRom != nullptr && ndsRom->GetHeader().IsDSiWare()) {
            // If we're trying to play a DSiWare game...
            InstallDsiware(mount, *ndsInfo); // Temporarily install the game on the NAND
            ndsRom = nullptr; // Don't want to insert the DSiWare into the cart slot
        }
    }

    melonDS::DSiArgs dsiargs {
        {
            .NDSROM = std::move(ndsRom),
            .GBAROM = nullptr, // Irrelevant on DSi
            .ARM9BIOS = std::move(arm9),
            .ARM7BIOS = std::move(arm7),
            .Firmware = std::move(*firmware),
        },
        std::move(arm9i),
        std::move(arm7i),
        std::move(nand),
        LoadDSiSDCardImage(config),
    };

    ApplyCommonArgs(config, dsiargs);

    return dsiargs;
}

static void MelonDsDs::ApplyCommonArgs(const CoreConfig& config, melonDS::NDSArgs& args) noexcept {
    ZoneScopedN(TracyFunction);
    args.Interpolation = config.Interpolation();
    args.BitDepth = config.BitDepth();
#ifdef JIT_ENABLED
    if (config.JitEnable()) {
        args.JIT = {
            .MaxBlockSize = config.MaxBlockSize(),
            .LiteralOptimizations = config.LiteralOptimizations(),
            .BranchOptimizations = config.BranchOptimizations(),
#   ifdef HAVE_JIT_FASTMEM
            .FastMemory = config.FastMemory(),
#   else
            .FastMemory = false,
#   endif
        };
    }
    else {
        args.JIT = std::nullopt;
    }
#else
    args.JIT = std::nullopt;
#endif
}

static unique_ptr<melonDS::NDSCart::CartCommon> MelonDsDs::LoadNdsCart(const CoreConfig& config, const retro::GameInfo& ndsInfo) {
    ZoneScopedN(TracyFunction);
    span<const std::byte> rom = ndsInfo.GetData();

    if (rom.size() < sizeof(NDSHeader)) {
        retro::error("ROM is only {} bytes, smaller than an NDS ROM header", rom.size());
        throw invalid_rom_exception("ROM is too small to be valid.");
    }

    const NDSHeader &header = *reinterpret_cast<const NDSHeader*>(rom.data());

    // These checks aren't comprehensive, but they should be good enough
    if (header.ARM9EntryAddress < 0x2000000 || header.ARM9EntryAddress > 0x23BFE00) {
        retro::error("Expected ARM9 entry address between 0x2000000 and 0x23BFE00, got 0x{:08x}", header.ARM9EntryAddress);
        throw invalid_rom_exception("ROM isn't valid, did you select the right file?");
    }

    if (header.NintendoLogoCRC16 != 0xCF56 && !header.IsHomebrew()) {
        retro::error("Expected logo CRC16 of 0xCF56, got 0x{:04x}", header.NintendoLogoCRC16);
        throw invalid_rom_exception("ROM isn't valid, did you select the right file?");
    }

    melonDS::NDSCart::NDSCartArgs sdargs = {
        .SDCard = config.DldiSdCardArgs(),
        .SRAM = nullptr, // SRAM is loaded separately by retro_get_memory
        .SRAMLength = 0,
    };

    std::unique_ptr<melonDS::NDSCart::CartCommon> cart;
    {
        ZoneScopedN("NDSCart::ParseROM");
        cart = melonDS::NDSCart::ParseROM(reinterpret_cast<const uint8_t*>(rom.data()), rom.size(), nullptr, std::move(sdargs));
    }

    if (!cart) {
        throw invalid_rom_exception("Failed to parse the DS ROM image. Is it valid?");
    }

    retro::debug("Parsed NDS ROM: \"{}\"", ndsInfo.GetPath());

    return cart;
}

static unique_ptr<melonDS::GBACart::CartCommon> MelonDsDs::LoadGbaCart(
    const retro::GameInfo& gbaInfo,
    const retro::GameInfo* gbaSaveInfo
) {
    ZoneScopedN(TracyFunction);

    unique_ptr<uint8_t[]> sram;
    size_t sramSize = 0;
    if (gbaSaveInfo) {
        auto result = LoadGbaSram(*gbaSaveInfo);
        sram = std::move(result.first);
        sramSize = result.second;
    }
    span<const std::byte> gbaRom = gbaInfo.GetData();

    unique_ptr<melonDS::GBACart::CartCommon> cart;
    {
        ZoneScopedN("GBACart::ParseROM");
        cart = melonDS::GBACart::ParseROM(
            reinterpret_cast<const uint8_t*>(gbaRom.data()),
            gbaRom.size(),
            sram.get(),
            sramSize
        );
    }

    if (!cart) {
        throw invalid_rom_exception("Failed to parse the GBA ROM image. Is it valid?");
    }

    retro::debug("Loaded GBA ROM: \"{}\"", gbaInfo.GetPath());

    return cart;
}

static std::pair<unique_ptr<uint8_t[]>, size_t> MelonDsDs::LoadGbaSram(const retro::GameInfo& gbaSaveInfo) {
    ZoneScopedN(TracyFunction);
    // We load the GBA SRAM file ourselves (rather than letting the frontend do it)
    // because we'll overwrite it later and don't want the frontend to hold open any file handles.
    // Due to libretro limitations, we can't use retro_get_memory_data to load the GBA SRAM
    // without asking the user to move their SRAM into the melonDS DS save folder.
    if (path_contains_compressed_file(gbaSaveInfo.GetPath().data())) {
        // If this save file is in an archive (e.g. /path/to/file.7z#mygame.srm)...

        // We don't support GBA SRAM files in archives right now;
        // libretro-common has APIs for extracting and re-inserting them,
        // but I just can't be bothered.
        retro::set_error_message(
            "melonDS DS does not support archived GBA save data right now. "
            "Please extract it and try again. "
            "Continuing without using the save data."
        );

        return { nullptr, 0 };
    }

    // rzipstream opens the file as-is if it's not rzip-formatted
    rzipstream_t* gba_save_file = rzipstream_open(gbaSaveInfo.GetPath().data(), RETRO_VFS_FILE_ACCESS_READ);
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
        return { nullptr, 0 };
    }

    int64_t gba_save_file_size = rzipstream_get_size(gba_save_file);
    if (gba_save_file_size < 0) {
        // If we couldn't get the uncompressed size of the GBA save file...
        rzipstream_close(gba_save_file);
        throw std::runtime_error("Failed to get GBA save file size");
    }

    unique_ptr<uint8_t[]> gba_save_data = make_unique<uint8_t[]>(gba_save_file_size);
    int64_t bytesRead = rzipstream_read(gba_save_file, gba_save_data.get(), gba_save_file_size);
    rzipstream_close(gba_save_file);
    if (bytesRead != gba_save_file_size) {
        throw std::runtime_error("Failed to read GBA save file");
    }

    retro::debug("Loaded {}-byte GBA SRAM from {}.", gba_save_file_size, gbaSaveInfo.GetPath());
    return {std::move(gba_save_data), gba_save_file_size};
}

void MelonDsDs::InstallDsiware(NANDMount& mount, const retro::GameInfo& nds_info) {
    ZoneScopedN(TracyFunction);
    std::string_view path = nds_info.GetPath();
    retro::info("Temporarily installing DSiWare title \"{}\" onto DSi NAND image", path);
    auto data = nds_info.GetData();
    const NDSHeader &header = *reinterpret_cast<const NDSHeader*>(data.data());
    retro_assert(header.IsDSiWare());

    if (mount.TitleExists(header.DSiTitleIDHigh, header.DSiTitleIDLow)) {
        retro::info("Title \"{}\" already exists on loaded NAND; skipping installation, and won't uninstall it later.", path);

    } else {
        retro::info("Title \"{}\" is not on loaded NAND; will install it for the duration of this session.", path);

        char tmd_path[PATH_MAX];
        GetTmdPath(nds_info, tmd_path);

        optional<TitleMetadata> tmd = GetCachedTmd(tmd_path);

        if (!tmd) {
            // If the TMD isn't available locally...

#ifdef HAVE_NETWORKING
            if (tmd = DownloadTmd(header, tmd_path); !tmd) {
                // ...then download it and save it to disk. If that didn't work...
                throw missing_metadata_exception("Cannot get title metadata for installation");
            }
#else
            throw missing_metadata_exception("Cannot get title metadata for installation, and this build does not support downloading it");
#endif
        }

        if (!mount.ImportTitle(reinterpret_cast<const uint8_t*>(data.data()), data.size(), *tmd, false)) {
            throw emulator_exception("Failed to import DSiWare title into NAND image");
        }

        ImportDsiwareSaveData(mount, nds_info, header, TitleData_PublicSav);
        ImportDsiwareSaveData(mount, nds_info, header, TitleData_PrivateSav);
        ImportDsiwareSaveData(mount, nds_info, header, TitleData_BannerSav);

        uint8_t zero = 0;
        auto sentinel = fmt::format("0:/title/{:08x}/{:08x}/data/{}", header.DSiTitleIDHigh, header.DSiTitleIDLow, SENTINEL_NAME);
        mount.RemoveFile(sentinel.c_str());
        mount.ImportFile(sentinel.c_str(), &zero, sizeof(zero));
    }
}

static void MelonDsDs::GetTmdPath(const retro::GameInfo &nds_info, std::span<char> buffer) {
    auto path = nds_info.GetPath();
    char tmd_name[PATH_MAX] {}; // "/path/to/game.zip#game.nds"
    const char *ptr = path_basename(path.data());  // "game.nds"
    strlcpy(tmd_name, ptr ? ptr : path.data(), sizeof(tmd_name));
    path_remove_extension(tmd_name); // "game"
    strlcat(tmd_name, ".tmd", sizeof(tmd_name)); // "game.tmd"

    optional<string_view> system_subdir = retro::get_system_subdirectory();
    if (!system_subdir) {
        throw emulator_exception("System directory not set");
    }

    char tmd_dir[PATH_MAX] {};
    fill_pathname_join_special(tmd_dir, system_subdir->data(), TMD_DIR_NAME, sizeof(tmd_dir));
    // "/libretro/system/melonDS DS/tmd"

    memset(buffer.data(), 0, buffer.size());
    fill_pathname_join_special(buffer.data(), tmd_dir, tmd_name, buffer.size());
    // "/libretro/system/melonDS DS/tmd/game.tmd"
}

static optional<TitleMetadata> MelonDsDs::GetCachedTmd(string_view tmdPath) noexcept {
    ZoneScopedN(TracyFunction);
    RFILE *tmd_file = filestream_open(tmdPath.data(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!tmd_file) {
        retro::info("Could not find local copy of title metadata at \"{}\"", tmdPath);
        return nullopt;
    }

    retro::info("Found title metadata at \"{}\"", tmdPath);
    TitleMetadata tmd {};
    int64_t bytes_read = filestream_read(tmd_file, &tmd, sizeof(TitleMetadata));
    filestream_close(tmd_file); // Not null, so it always succeeds

    if (bytes_read < 0) {
        // If there was an error reading the file...
        retro::error("Error reading title metadata");
        return nullopt;
    }

    if (static_cast<size_t>(bytes_read) < sizeof(TitleMetadata)) {
        // If the file was too small...
        retro::error("Title metadata file is too small, it may be corrupt");
        return nullopt;
    }

    if (!ValidateTmd(tmd)) {
        // If the file is corrupt...
        retro::error("Title metadata validation failed; the file is corrupt");
        return nullopt;
    }

    retro::info("Title metadata OK");

    return tmd;
}

static bool MelonDsDs::ValidateTmd(const TitleMetadata &tmd) noexcept {
    if (tmd.SignatureType != RSA256_SIGNATURE_TYPE) {
        retro::error("Invalid signature type {:#x}", tmd.SignatureType);
        return false;
    }

    return true;
}

static optional<TitleMetadata> MelonDsDs::DownloadTmd(const NDSHeader &header, string_view tmdPath) noexcept {
    ZoneScopedN(TracyFunction);
    auto url = fmt::format(
        "http://nus.cdn.t.shop.nintendowifi.net/ccs/download/{:08x}{:08x}/tmd",
        header.DSiTitleIDHigh,
        header.DSiTitleIDLow
    );
    // The URL comes from here https://problemkaputt.de/gbatek.htm#dsisdmmcdsiwarefilesfromnintendosserver
    // Example: http://nus.cdn.t.shop.nintendowifi.net/ccs/download/00030015484e4250/tmd

    retro::info("Downloading title metadata from \"{}\"", url);

    // Create and send the HTTP request
    retro::HttpConnection connection(url, "GET");

    size_t progress = 0, total = 0;
    while (!connection.Update(progress, total)) {
        retro_sleep(20);
        // TODO: Use select with a timeout instead of a busy loop
    }

    if (connection.IsError()) {
        // If there was a problem...
        if (int status = connection.Status(); status > 0) {
            // ...but we did manage to get a status code...
            retro::error("HTTP request to {} failed with {}", url, connection.Status());
        } else {
            retro::error("HTTP request to {} failed with unknown error", url);
        }

        return nullopt;
    }

    // If the request succeeded, get the payload
    span<const std::byte> payload = connection.Data(false);
    if (payload.empty()) {
        // If there was no payload...
        retro::error("HTTP request to {} succeeded, but it sent no data", url);
        return nullopt;
    }

    if (payload.size() < sizeof(TitleMetadata)) {
        // Or if the payload was too small...
        retro::error(
            "HTTP request to {} returned a response of {} bytes, expected one at least {} bytes long",
            url,
            payload.size(),
            sizeof(TitleMetadata)
        );

        return nullopt;
    }

    // It's okay if the payload is too big; we don't need the entire TMD
    retro::info("HTTP request succeeded with {} bytes", payload.size());
    TitleMetadata tmd {};
    memcpy(&tmd, payload.data(), sizeof(TitleMetadata));

    if (!ValidateTmd(tmd)) {
        // If the TMD isn't what we expected...
        retro::error("Title metadata validation failed; the server sent invalid data");
        return nullopt;
    }

    retro::info("Downloaded TMD successfully");

    CacheTmd(tmdPath, payload);

    return tmd;
}

static bool MelonDsDs::CacheTmd(string_view tmd_path, std::span<const std::byte> tmd) noexcept {
    ZoneScopedN(TracyFunction);
    char tmd_dir[PATH_MAX];
    strlcpy(tmd_dir, tmd_path.data(), sizeof(tmd_dir));
    path_basedir(tmd_dir);

    if (!path_mkdir(tmd_dir)) {
        retro::error("Error creating title metadata directory \"{}\"", tmd_dir);
        return false;
    }

    if (filestream_write_file(tmd_path.data(), tmd.data(), tmd.size())) {
        retro::info("Cached title metadata to \"{}\"", tmd_path);
        return true;
    } else {
        retro::error("Error writing title metadata to \"{}\"", tmd_path);
        return false;
    }
}

static void MelonDsDs::ImportDsiwareSaveData(NANDMount& nand, const retro::GameInfo& nds_info, const NDSHeader& header, int type) noexcept {
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

    if (path_stat(sav_file) != RETRO_VFS_STAT_IS_VALID) {
        // If this path is not a valid file...
        retro::info("No DSiWare save data found at \"{}\"", sav_file);
    } else if (nand.ImportTitleData(header.DSiTitleIDHigh, header.DSiTitleIDLow, type, sav_file)) {
        retro::info("Imported DSiWare save data from \"{}\"", sav_file);
    } else {
        retro::warn("Couldn't import DSiWare save data from \"{}\"", sav_file);
    }
}

bool MelonDsDs::GetDsiwareSaveDataHostPath(std::span<char> buffer, const retro::GameInfo& nds_info, int type) noexcept {
    if (buffer.empty()) {
        return false;
    }

    optional<string_view> save_directory = retro::get_save_directory();
    if (!save_directory) {
        retro::error("Save directory not available, cannot import DSiWare save data");
        return false;
    }

    char sav_name[PATH_MAX] {}; // "/path/to/game.zip#game.nds"
    const char *ptr = path_basename(nds_info.GetPath().data());  // "game.nds"
    strlcpy(sav_name, ptr ? ptr : nds_info.GetPath().data(), sizeof(sav_name));
    path_remove_extension(sav_name); // "game"
    switch (type) {
        case TitleData_PublicSav:
            strlcat(sav_name, ".public.sav", sizeof(sav_name)); // "game.public.sav"
            break;
        case TitleData_PrivateSav:
            strlcat(sav_name, ".private.sav", sizeof(sav_name)); // "game.private.sav"
            break;
        case TitleData_BannerSav:
            strlcat(sav_name, ".banner.sav", sizeof(sav_name)); // "game.banner.sav"
            break;
        default:
            retro::error("Unknown save type {}", type);
            return false;
    }

    fill_pathname_join_special(buffer.data(), save_directory->data(), sav_name, buffer.size());
    // "/path/to/saves/game.public.sav"
    return true;
}

static bool MelonDsDs::LoadBios(const string_view& name, BiosType type, std::span<uint8_t> buffer) noexcept {
    ZoneScopedN(TracyFunction);

    auto LoadBiosImpl = [&](const string& path) -> bool {
        RFILE* file = filestream_open(path.c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);

        if (!file) {
            retro::error("Failed to open {} file \"{}\" for reading", type, path);
            return false;
        }

        if (int64_t size = filestream_get_size(file); size != buffer.size()) {
            retro::error("Expected {} file \"{}\" to be exactly {} bytes long, got {} bytes", type, path, buffer.size(),
                         size);
            filestream_close(file);
            return false;
        }

        if (int64_t bytesRead = filestream_read(file, buffer.data(), buffer.size()); bytesRead != buffer.size()) {
            retro::error("Expected to read {} bytes from {} file \"{}\", got {} bytes", buffer.size(), type, path,
                         bytesRead);
            filestream_close(file);
            return false;
        }

        filestream_close(file);
        retro::info("Successfully loaded {}-byte {} file \"{}\"", buffer.size(), type, path);

        return true;
    };

    // Prefer looking in "system/melonDS DS/${name}", but fall back to "system/${name}" if that fails

    if (optional<string> path = retro::get_system_subdir_path(name); path && LoadBiosImpl(*path)) {
        // Get the path where we're expecting a BIOS file. If it's there and we loaded it...
        return true;
    }

    if (optional<string> path = retro::get_system_path(name); path && LoadBiosImpl(*path)) {
        // Get the path where we're expecting a BIOS file. If it's there and we loaded it...
        return true;
    }

    retro::error("Failed to load {} file \"{}\"", type, name);

    return false;
}

/// Loads firmware, does not patch it.
static optional<Firmware> MelonDsDs::LoadFirmware(const string& firmwarePath) noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs;
    using namespace MelonDsDs::config::firmware;

    // Try to open the configured firmware dump.
    RFILE* file = filestream_open(firmwarePath.c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!file) {
        // If that fails...
        retro::error("Failed to open firmware file \"{}\" for reading", firmwarePath);
        return nullopt;
    }

    int64_t fileSize = filestream_get_size(file);
    unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(fileSize);
    int64_t bytesRead = filestream_read(file, buffer.get(), fileSize);
    filestream_close(file);

    if (bytesRead != fileSize) {
        // If we couldn't read the firmware file...
        retro::error("Failed to read firmware file \"{}\"; got {} bytes, expected {} bytes", firmwarePath, bytesRead,
                     fileSize);
        return nullopt;
    }

    // Try to load the firmware dump into the object.
    optional<Firmware> firmware = std::make_optional<Firmware>(buffer.get(), fileSize);

    if (!firmware->Buffer()) {
        // If we failed to load the firmware...
        retro::error("Failed to read opened firmware file \"{}\"", firmwarePath);
        return nullopt;
    }

    melonDS::FirmwareIdentifier id = firmware->GetHeader().Identifier;
    Firmware::FirmwareConsoleType type = firmware->GetHeader().ConsoleType;
    retro::info(
        "Loaded {} firmware from \"{}\" (Identifier: {})",
        type,
        firmwarePath,
        string_view(reinterpret_cast<const char*>(id.data()), 4)
    );

    return firmware;
}

static void MelonDsDs::CustomizeFirmware(const CoreConfig& config, Firmware& firmware) {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::firmware;

    // We don't need to save the whole firmware, just the part that may actually change.
    optional<string> wfcsettingspath = retro::get_system_subdir_path(config.GeneratedFirmwareSettingsPath());
    if (!wfcsettingspath) {
        throw environment_exception("No system directory is available");
    }

    const Firmware::FirmwareHeader& header = firmware.GetHeader();

    // If using generated firmware, we keep the wi-fi settings on the host disk separately.
    // Wi-fi access point data includes Nintendo WFC settings,
    // and if we didn't keep them then the player would have to reset them in each session.
    if (retro::rfile_ptr file = retro::make_rfile(*wfcsettingspath, RETRO_VFS_FILE_ACCESS_READ)) {
        // If we have Wi-fi settings to load...
        constexpr unsigned TOTAL_WFC_SETTINGS_SIZE =
            3 * (sizeof(Firmware::WifiAccessPoint) + sizeof(Firmware::ExtendedWifiAccessPoint));

        // The access point and extended access point segments might
        // be in different locations depending on the firmware revision,
        // but our generated firmware always keeps them next to each other.
        // (Extended access points first, then regular ones.)
        uint8_t* userdata = firmware.GetExtendedAccessPointPosition();

        if (filestream_read(file.get(), userdata, TOTAL_WFC_SETTINGS_SIZE) != TOTAL_WFC_SETTINGS_SIZE) {
            // If we couldn't read the Wi-fi settings from this file...
            retro::warn("Failed to read Wi-fi settings from \"{}\"; using defaults instead\n", *wfcsettingspath);

            firmware.GetAccessPoints() = {
                Firmware::WifiAccessPoint(header.ConsoleType == Firmware::FirmwareConsoleType::DSi ? 1 : 0),
                Firmware::WifiAccessPoint(),
                Firmware::WifiAccessPoint(),
            };

            firmware.GetExtendedAccessPoints() = {
                Firmware::ExtendedWifiAccessPoint(),
                Firmware::ExtendedWifiAccessPoint(),
                Firmware::ExtendedWifiAccessPoint(),
            };
        }
    }
    else {
        retro::info("No existing Wi-fi settings found at {}\n", *wfcsettingspath);
    }

    // If we don't have Wi-fi settings to load,
    // then the defaults will have already been populated by the constructor.

    if (header.Identifier != melonDS::GENERATED_FIRMWARE_IDENTIFIER && header.ConsoleType ==
        Firmware::FirmwareConsoleType::DS) {
        // If we're using externally-loaded DS (not DSi) firmware...

        uint8_t chk1[0x180], chk2[0x180];

        // I don't really know how this works, it's just adapted from upstream
        memcpy(chk1, firmware.Buffer(), sizeof(chk1));
        memcpy(chk2, firmware.Buffer() + firmware.Length() - 0x380, sizeof(chk2));

        memset(&chk1[0x0C], 0, 8);
        memset(&chk2[0x0C], 0, 8);

        if (!memcmp(chk1, chk2, sizeof(chk1))) {
            constexpr const char* const WARNING_MESSAGE =
                "Corrupted firmware detected!\n"
                "Any game that alters Wi-fi settings will break this firmware, even on real hardware.\n";

            if (config.ShowBiosWarnings()) {
                retro::set_warn_message(WARNING_MESSAGE);
            }
            else {
                retro::warn(WARNING_MESSAGE);
            }
        }
    }

    Firmware::UserData& currentData = firmware.GetEffectiveUserData();

    // setting up username
    if (config.UsernameMode() != UsernameMode::Firmware) {
        // If we want to override the existing username...
        optional<string> username = GetUsername(config.UsernameMode());
        if (!username || username->empty()) {
            retro::set_warn_message("Failed to get username, or none was provided; using default");
            username = config::values::firmware::DEFAULT_USERNAME;
        }

        optional<u16string> convertedUsername = ConvertUsername(*username);
        if (!convertedUsername) {
            retro::set_warn_message("Can't use the name \"{}\" on the DS, using default name instead.", *username);
            convertedUsername = ConvertUsername(config::values::firmware::DEFAULT_USERNAME);
        }

        size_t usernameLength = std::min(convertedUsername->length(), (size_t)config::DS_NAME_LIMIT);
        currentData.NameLength = usernameLength;

        memset(currentData.Nickname, 0, sizeof(currentData.Nickname));
        memcpy(currentData.Nickname, convertedUsername->data(), usernameLength * sizeof(char16_t));
    }

    switch (config.Language()) {
        case FirmwareLanguage::Auto:

            if (optional<retro_language> retroLanguage = retro::get_language()) {
                currentData.Settings &= ~Firmware::Language::Reserved; // clear the existing language bits
                currentData.Settings |= GetFirmwareLanguage(*retroLanguage);
            }
            else {
                retro::warn("Failed to get language from frontend; defaulting to existing firmware value");
            }
            break;
        case FirmwareLanguage::Default:
            // do nothing, leave the existing language in place
            break;
        default:
            currentData.Settings &= ~Firmware::Language::Reserved;
            currentData.Settings |= static_cast<Firmware::Language>(config.Language());
            break;
    }

    if (config.FavoriteColor() != Color::Default) {
        currentData.FavoriteColor = static_cast<uint8_t>(config.FavoriteColor());
    }

    if (auto birthday = config.Birthday()) {
        // If the frontend specifies a birth date and month (rather than using the existing value)...
        currentData.BirthdayMonth = static_cast<unsigned>(birthday->month());
        currentData.BirthdayDay = static_cast<unsigned>(birthday->day());
    }

    if (auto alarm = config.Alarm()) {
        // If the frontend specifies an alarm time (rather than using the existing value)...
        currentData.AlarmHour = alarm->hours().count();
        currentData.AlarmMinute = alarm->minutes().count();
    }

    if (auto dns = config.DnsServer()) {
        firmware.GetAccessPoints()[0].PrimaryDns = *dns;
        firmware.GetAccessPoints()[0].SecondaryDns = *dns;
    }

    if (auto address = config.MacAddress()) {
        melonDS::MacAddress mac = *address;
        mac[0] &= 0xFC; // ensure the MAC isn't a broadcast MAC
        firmware.GetHeader().MacAddr = mac;
    }

    // fix touchscreen coords
    currentData.TouchCalibrationADC1[0] = 0;
    currentData.TouchCalibrationADC1[1] = 0;
    currentData.TouchCalibrationPixel1[0] = 0;
    currentData.TouchCalibrationPixel1[1] = 0;
    currentData.TouchCalibrationADC2[0] = 255 << 4;
    currentData.TouchCalibrationADC2[1] = 191 << 4;
    currentData.TouchCalibrationPixel2[0] = 255;
    currentData.TouchCalibrationPixel2[1] = 191;

    firmware.UpdateChecksums();
}

static std::optional<std::string> MelonDsDs::GetUsername(UsernameMode mode) noexcept {
    ZoneScopedN(TracyFunction);
    using namespace config;

    switch (mode) {
        case UsernameMode::Guess:
            if (optional<string_view> frontendGuess = retro::username(); frontendGuess && !frontendGuess->empty()) {
                return string(*frontendGuess);
            }

            if (const char* user = getenv("USER"); !string_is_empty(user)) {
                return user;
            }

            if (const char* username = getenv("USERNAME"); !string_is_empty(username)) {
                return username;
            }

            if (const char* logname = getenv("LOGNAME"); !string_is_empty(logname)) {
                return logname;
            }

            return nullopt;
        case UsernameMode::MelonDSDS:
        default:
            return values::firmware::DEFAULT_USERNAME;
    }

    return nullopt;
}

static std::optional<std::u16string> MelonDsDs::ConvertUsername(string_view str) noexcept {
    ZoneScopedN(TracyFunction);
    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter {};
    try {
        u16string converted = converter.from_bytes(str.data());

        if (converted.length() > config::DS_NAME_LIMIT) {
            // If the converted name is too long...
            converted.resize(config::DS_NAME_LIMIT);
        }

        if (converted.find_first_not_of(config::NdsCharacterSet) != u16string::npos) {
            // If the converted name has a character we can't use...
            retro::error("Converted {} to UCS-2, but it contains characters that can't be used on the DS", str);
            return nullopt;
        }

        return converted;
    } catch (const std::range_error& e) {
        retro::error("Failed to convert {} to UCS-2: {}", str, e.what());
        return nullopt;
    }
}

/// Loads the DSi NAND, does not patch it
static NANDImage MelonDsDs::LoadNANDImage(const string& nandPath, const uint8_t* es_keyY) {
    ZoneScopedN(TracyFunction);
    using namespace melonDS::Platform;
    FileHandle* nandFile = OpenLocalFile(nandPath, FileMode::ReadWriteExisting);
    if (!nandFile) {
        throw dsi_nand_missing_exception(nandPath);
    }

    NANDImage nand(nandFile, es_keyY);
    if (!nand) {
        throw dsi_nand_corrupted_exception(nandPath);
    }
    retro::debug("Opened the DSi NAND image file at {}", nandPath);

    return nand;
}

static void MelonDsDs::CustomizeNAND(const CoreConfig& config, NANDMount& mount, const NDSHeader* header, string_view nandName) {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::system;
    using namespace MelonDsDs::config::firmware;

    DSiSerialData dataS {};
    memset(&dataS, 0, sizeof(dataS));
    if (!mount.ReadSerialData(dataS)) {
        throw emulator_exception("Failed to read serial data from NAND image");
    }

    if (header && header->IsDSiWare()) {
        // If we're loading a DSiWare game...

        uint32_t consoleRegionMask = (1 << (int)dataS.Region);
        if (!(consoleRegionMask & header->DSiRegionMask)) {
            // If the console's region isn't compatible with the game's regions...
            throw dsi_region_mismatch_exception(nandName, dataS.Region, header->DSiRegionMask);
        }

        retro::debug("Console region ({}) and game regions ({}) match", dataS.Region, header->DSiRegionMask);
    }

    DSiFirmwareSystemSettings settings;
    if (!mount.ReadUserData(settings)) {
        throw emulator_exception("Failed to read user data from NAND image");
    }

    // Right now, I only modify the user data with the firmware overrides defined by core options
    // If there are any problems, I may want to completely synchronize the user data and firmware myself.

    // setting up username
    if (config.UsernameMode() != UsernameMode::Firmware) {
        // If we want to override the existing username...
        optional<string> username = GetUsername(config.UsernameMode());
        if (!username || username->empty()) {
            username = config::values::firmware::DEFAULT_USERNAME;
        }

        optional<u16string> convertedUsername = ConvertUsername(*username);
        if (!convertedUsername) {
            convertedUsername = ConvertUsername(config::values::firmware::DEFAULT_USERNAME);
        }

        size_t usernameLength = std::min(convertedUsername->length(), (size_t)config::DS_NAME_LIMIT);

        memset(settings.Nickname, 0, sizeof(settings.Nickname));
        memcpy(settings.Nickname, convertedUsername->data(), usernameLength * sizeof(char16_t));
    }

    switch (config.Language()) {
        case FirmwareLanguage::Auto:
            if (optional<retro_language> retroLanguage = retro::get_language()) {
                // If we can't query the frontend's language, just leave that part of the firmware alone
                Firmware::Language firmwareLanguage = GetFirmwareLanguage(*retroLanguage);
                if (dataS.SupportedLanguages & (1 << firmwareLanguage)) {
                    // If the NAND supports the frontend's language...
                    settings.Language = firmwareLanguage;
                    settings.ConfigFlags |= (1 << 2); // LanguageSet? (usually 1) flag
                } else {
                    retro::warn("The frontend's preferred language ({}) isn't supported by this NAND image; not overriding it.", *retroLanguage);
                }
            } else {
                retro::warn("Can't query the frontend's preferred language, not overriding it.");
            }

            break;

        case FirmwareLanguage::Default:
            // do nothing, leave the existing language in place
            break;
        default: {
            Firmware::Language firmwareLanguage = static_cast<Firmware::Language>(config.Language());
            if (dataS.SupportedLanguages & (1 << firmwareLanguage)) {
                // If the NAND supports the core option's specified language...
                settings.Language = firmwareLanguage;
                settings.ConfigFlags |= (1 << 2); // LanguageSet? (usually 1) flag
            } else {
                retro::warn("The configured language ({}) is not supported by this NAND image; not overriding it.", firmwareLanguage);
            }

        }
            break;
    }
    settings.ConfigFlags |= (1 << 24); // EULA flag (agreed)

    if (config.FavoriteColor() != Color::Default) {
        settings.FavoriteColor = static_cast<uint8_t>(config.FavoriteColor());
    }

    if (auto birthday = config.Birthday()) {
        // If the frontend specifies a birthday (rather than using the existing value)...
        settings.BirthdayMonth = static_cast<unsigned>(birthday->month());
        settings.BirthdayDay = static_cast<unsigned>(birthday->day());
    }

    if (config.AlarmMode() != AlarmMode::Default) {
        settings.AlarmEnable = (config.AlarmMode() == AlarmMode::Enabled);
    }

    if (auto alarm = config.Alarm()) {
        settings.AlarmHour = alarm->hours().count();
        settings.AlarmMinute = alarm->minutes().count();
    }

    if (header && header->IsDSiWare()) {
        // If we're loading a DSiWare game...

        memcpy(&settings.SystemMenuMostRecentTitleID[0], &header->DSiTitleIDLow, sizeof(header->DSiTitleIDLow));
        memcpy(&settings.SystemMenuMostRecentTitleID[4], &header->DSiTitleIDHigh, sizeof(header->DSiTitleIDHigh));
    }

    // The DNS entries and MAC address aren't stored on the NAND,
    // so we don't need to try to update them here.

    // fix touchscreen coords
    settings.TouchCalibrationADC1 = {0, 0};
    settings.TouchCalibrationPixel1 = {0, 0};
    settings.TouchCalibrationADC2 = {255 << 4, 191 << 4};
    settings.TouchCalibrationPixel2 = {255, 191};

    settings.UpdateHash();

    if (!mount.ApplyUserData(settings)) {
        throw emulator_exception("Failed to write user data to NAND image");
    }
}

static optional<melonDS::FATStorage> MelonDsDs::LoadDSiSDCardImage(const CoreConfig& config) noexcept {
    ZoneScopedN(TracyFunction);
    if (!config.DsiSdEnable()) return nullopt;

    return melonDS::FATStorage(
        string(config.DsiSdImagePath()),
        config.DsiSdImageSize(),
        config.DsiSdReadOnly(),
        config.DsiSdFolderSync() ? make_optional(string(config.DsiSdFolderPath())) : nullopt
    );
}