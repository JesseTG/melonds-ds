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
#include <string_view>

#include <FreeBIOS.h>
#include <NDS.h>
#include <DSi.h>

#include <retro_assert.h>
#include <streams/file_stream.h>
#include <string/stdstring.h>

#include "config.hpp"
#include "environment.hpp"
#include "exceptions.hpp"
#include "retro/info.hpp"
#include "types.hpp"

using std::make_optional;
using std::make_unique;
using std::nullopt;
using std::optional;
using std::string_view;
using std::unique_ptr;
using melonDS::Firmware;
using melonDS::NDSHeader;
using namespace melonDS::DSi_NAND;

namespace MelonDsDs {
    static melonDS::NDSArgs GetNdsArgs(
        const CoreConfig& config,
        const retro::GameInfo* ndsInfo,
        const retro::GameInfo* gbaInfo,
        const retro::GameInfo* gbaSaveInfo
    );
    static melonDS::DSiArgs GetDSiArgs(const CoreConfig& config, const retro::GameInfo* ndsInfo);
    static void ApplyCommonArgs(const CoreConfig& config, melonDS::NDSArgs& args) noexcept;
    static optional<Firmware> LoadFirmware(const string& firmwarePath) noexcept;
    static bool LoadBios(const string_view& name, BiosType type, std::span<uint8_t> buffer) noexcept;
    static void CustomizeFirmware(const CoreConfig& config, Firmware& firmware);
    static std::string GetUsername(UsernameMode mode) noexcept;
    static NANDImage LoadNANDImage(const string& nandPath, const uint8_t* es_keyY);
    static void CustomizeNAND(const CoreConfig& config, NANDMount& mount, const NDSHeader* header, string_view nandName);
    static optional<melonDS::FATStorage> LoadDSiSDCardImage(const CoreConfig& config) noexcept;
    static std::u16string ConvertToUTF16(string_view str);

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
        return std::make_unique<melonDS::DSi>(GetDSiArgs(config, ndsInfo));
    }
    else {
        // If we're in DS mode...
        return std::make_unique<melonDS::NDS>(GetNdsArgs(config, ndsInfo, gbaInfo, gbaSaveInfo));
    }
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
    const retro::GameInfo* gbaSaveInfo
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

    if (config.SysfileMode() == SysfileMode::BuiltIn) {
        retro::debug("Not loading native ARM BIOS files");
    }

    melonDS::NDSArgs ndsargs {};

    ApplyCommonArgs(config, ndsargs);

    // Try to load the ARM7 and ARM9 BIOS files (but don't bother with the ARM9 BIOS if the ARM7 BIOS failed)
    bool bios7Loaded = (config.SysfileMode() == SysfileMode::Native) && LoadBios(
                           config.Bios7Path(), BiosType::Arm7, ndsargs.ARM7BIOS);
    bool bios9Loaded = bios7Loaded && LoadBios(config.Bios9Path(), BiosType::Arm9, ndsargs.ARM9BIOS);

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
        ndsargs.ARM9BIOS = melonDS::bios_arm9_bin;
        ndsargs.ARM7BIOS = melonDS::bios_arm7_bin;
        retro::debug("Installed built-in ARM7 and ARM9 NDS BIOS images");
    }

    CustomizeFirmware(config, *firmware);
    ndsargs.Firmware = std::move(*firmware);

    return ndsargs;
}

static melonDS::DSiArgs MelonDsDs::GetDSiArgs(const CoreConfig& config, const retro::GameInfo* ndsInfo) {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config::system;
    using namespace MelonDsDs::config::firmware;

    retro_assert(config.ConsoleType() == ConsoleType::DSi);

    string_view nandName = config.DsiNandPath();
    if (nandName == config::values::NOT_FOUND) {
        throw dsi_no_nand_found_exception();
    }

    if (config.DsiFirmwarePath() == config::values::NOT_FOUND) {
        throw dsi_no_firmware_found_exception();
    }

    // DSi mode requires all native BIOS files
    std::array<uint8_t, melonDS::DSiBIOSSize> arm7i;
    if (!LoadBios(config.DsiBios7Path(), BiosType::Arm7i, arm7i)) {
        throw dsi_missing_bios_exception(BiosType::Arm7i, config.DsiBios7Path());
    }

    std::array<uint8_t, melonDS::DSiBIOSSize> arm9i;
    if (!LoadBios(config.DsiBios9Path(), BiosType::Arm9i, arm9i)) {
        throw dsi_missing_bios_exception(BiosType::Arm9i, config.DsiBios9Path());
    }

    std::array<uint8_t, melonDS::ARM7BIOSSize> arm7;
    if (!LoadBios(config.Bios7Path(), BiosType::Arm7, arm7)) {
        throw dsi_missing_bios_exception(BiosType::Arm7, config.Bios7Path());
    }

    std::array<uint8_t, melonDS::ARM9BIOSSize> arm9;
    if (!LoadBios(config.Bios9Path(), BiosType::Arm9, arm9)) {
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

    NANDImage nand = LoadNANDImage(*nandPath, &arm7i[0x8308]);

    { // Scoped to limit the mount's lifetime
        NANDMount mount(nand);
        if (!mount) {
            throw dsi_nand_corrupted_exception(nandName);
        }
        retro::debug("Opened and mounted the DSi NAND image file at {}", *nandPath);

        const NDSHeader* header = ndsInfo ? reinterpret_cast<const NDSHeader*>(ndsInfo->GetData().data()) : nullptr;
        CustomizeNAND(config, mount, header, nandName);
    }

    melonDS::DSiArgs dsiargs {
        {
            .NDSROM = nullptr, // Inserted later
            .GBAROM = nullptr, // Irrelevant on DSi
            .ARM9BIOS = arm9,
            .ARM7BIOS = arm7,
            .Firmware = std::move(*firmware),
        },
        arm9i,
        arm7i,
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
#endif
        };
    }
    else {
        args.JIT = std::nullopt;
    }
#else
    args.JIT = std::nullopt;
#endif
}


static bool MelonDsDs::LoadBios(const string_view& name, MelonDsDs::BiosType type, std::span<uint8_t> buffer) noexcept {
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
    if (RFILE* file = filestream_open(wfcsettingspath->c_str(), RETRO_VFS_FILE_ACCESS_READ,
                                      RETRO_VFS_FILE_ACCESS_HINT_NONE)) {
        // If we have Wi-fi settings to load...
        constexpr unsigned TOTAL_WFC_SETTINGS_SIZE =
            3 * (sizeof(Firmware::WifiAccessPoint) + sizeof(Firmware::ExtendedWifiAccessPoint));

        // The access point and extended access point segments might
        // be in different locations depending on the firmware revision,
        // but our generated firmware always keeps them next to each other.
        // (Extended access points first, then regular ones.)
        uint8_t* userdata = firmware.GetExtendedAccessPointPosition();

        if (filestream_read(file, userdata, TOTAL_WFC_SETTINGS_SIZE) != TOTAL_WFC_SETTINGS_SIZE) {
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

        filestream_close(file);
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
        std::u16string username = ConvertToUTF16(GetUsername(config.UsernameMode()));
        size_t usernameLength = std::min(username.length(), (size_t)config::DS_NAME_LIMIT);
        currentData.NameLength = usernameLength;

        memcpy(currentData.Nickname, username.data(), usernameLength * sizeof(char16_t));
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

static std::string MelonDsDs::GetUsername(UsernameMode mode) noexcept {
    ZoneScopedN(TracyFunction);
    using namespace config;
    char result[DS_NAME_LIMIT + 1];
    result[DS_NAME_LIMIT] = '\0';

    switch (mode) {
        case UsernameMode::Firmware:
            return values::firmware::FIRMWARE_USERNAME;
        case UsernameMode::Guess: {
            if (optional<string> frontendGuess = retro::username(); frontendGuess && !frontendGuess->empty()) {
                return *frontendGuess;
            }
            else if (const char* user = getenv("USER"); !string_is_empty(user)) {
                strncpy(result, user, DS_NAME_LIMIT);
            }
            else if (const char* username = getenv("USERNAME"); !string_is_empty(username)) {
                strncpy(result, username, DS_NAME_LIMIT);
            }
            else if (const char* logname = getenv("LOGNAME"); !string_is_empty(logname)) {
                strncpy(result, logname, DS_NAME_LIMIT);
            }
            else {
                strncpy(result, values::firmware::DEFAULT_USERNAME, DS_NAME_LIMIT);
            }

            return result;
        }
        case UsernameMode::MelonDSDS:
        default:
            return values::firmware::DEFAULT_USERNAME;
    }
}

static std::u16string MelonDsDs::ConvertToUTF16(string_view str) {
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter {};
    return converter.from_bytes(str.data());
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
        std::u16string username = ConvertToUTF16(GetUsername(config.UsernameMode()));
        size_t usernameLength = std::min(username.length(), (size_t)config::DS_NAME_LIMIT);

        memset(settings.Nickname, 0, sizeof(settings.Nickname));
        memcpy(settings.Nickname, username.data(), usernameLength * sizeof(char16_t));
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