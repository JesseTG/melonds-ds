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

#ifndef MELONDS_DS_CONFIG_HPP
#define MELONDS_DS_CONFIG_HPP

#undef isnan

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <memory>
#include <span>
#include <SPI_Firmware.h>
#include <string>
#include <string_view>
#include <SPU.h>

#include "parse.hpp"
#include "definitions.hpp"
#include "types.hpp"

namespace melonDS {
    struct NDSArgs;
    struct DSiArgs;
    struct NDSHeader;
    struct RenderSettings;
    class Firmware;
    class NDS;
}

namespace MelonDsDs {
    struct CoreState;
}

struct retro_core_options_v2;
struct retro_core_option_v2_definition;
struct retro_game_info;

namespace MelonDsDs {
    class ScreenLayoutData;
    class InputState;
    class CoreConfig;

    /// Called when loading a game
    [[deprecated("Split into LoadConfig and ApplyConfig")]] void InitConfig(
        MelonDsDs::CoreState& core,
        const melonDS::NDSHeader* header, // I'd like to have an optional<NDSHeader&>, but C++ doesn't allow it
        ScreenLayoutData& screenLayout,
        InputState& inputState
    );

    void ParseConfig(CoreConfig& config) noexcept;

    /// Called when settings have been updated mid-game
    void UpdateConfig(MelonDsDs::CoreState& core, ScreenLayoutData& screenLayout, InputState& inputState) noexcept;

    [[nodiscard]] bool RegisterCoreOptions() noexcept;

    using std::string;
    using std::string_view;
    using std::optional;
    using namespace std::chrono;

    class CoreConfig {
    public:
        [[nodiscard]] MicButtonMode MicButtonMode() const noexcept { return _micButtonMode; }
        void SetMicButtonMode(MelonDsDs::MicButtonMode mode) noexcept { _micButtonMode = mode; }

        [[nodiscard]] MicInputMode MicInputMode() const noexcept { return _micInputMode; }
        void SetMicInputMode(MelonDsDs::MicInputMode mode) noexcept { _micInputMode = mode; }

        [[nodiscard]] melonDS::AudioBitDepth BitDepth() const noexcept { return _bitDepth; }
        void SetBitDepth(melonDS::AudioBitDepth bitDepth) noexcept { _bitDepth = bitDepth; }

        [[nodiscard]] melonDS::AudioInterpolation Interpolation() const noexcept { return _interpolation; }
        void SetInterpolation(melonDS::AudioInterpolation interpolation) noexcept { _interpolation = interpolation; }

        [[nodiscard]] AlarmMode AlarmMode() const noexcept { return _alarmMode; }
        void SetAlarmMode(MelonDsDs::AlarmMode alarmMode) noexcept { _alarmMode = alarmMode; }

        [[nodiscard]] optional<unsigned> AlarmHour() const noexcept { return _alarmHour; }
        void SetAlarmHour(optional<unsigned> hour) noexcept { _alarmHour = hour; }

        [[nodiscard]] optional<unsigned> AlarmMinute() const noexcept { return _alarmMinute; }
        void SetAlarmMinute(optional<unsigned> minute) noexcept { _alarmMinute = minute; }

        [[nodiscard]] optional<hh_mm_ss<minutes>> Alarm() const noexcept {
            if (!_alarmHour.has_value() || !_alarmMinute.has_value() || _alarmMode != AlarmMode::Enabled) {
                return std::nullopt;
            }

            return hh_mm_ss(hours(*_alarmHour) + minutes(*_alarmMinute));
        }

        [[nodiscard]] FirmwareLanguage Language() const noexcept { return _language; }
        void SetLanguage(FirmwareLanguage language) noexcept { _language = language; }

        [[nodiscard]] unsigned BirthdayMonth() const noexcept { return _birthdayMonth; }
        void SetBirthdayMonth(unsigned month) noexcept { _birthdayMonth = month; }

        [[nodiscard]] unsigned BirthdayDay() const noexcept { return _birthdayDay; }
        void SetBirthdayDay(unsigned day) noexcept { _birthdayDay = day; }

        [[nodiscard]] optional<month_day> Birthday() const noexcept {
            return (_birthdayDay > 0 && _birthdayMonth > 0) ? month(_birthdayMonth) / day(_birthdayDay) : std::nullopt;
        }

        [[nodiscard]] Color FavoriteColor() const noexcept { return _favoriteColor; }
        void SetFavoriteColor(Color color) noexcept { _favoriteColor = color; }

        [[nodiscard]] MelonDsDs::UsernameMode UsernameMode() const noexcept { return _usernameMode; }
        void SetUsernameMode(MelonDsDs::UsernameMode mode) noexcept { _usernameMode = mode; }

        [[nodiscard]] string_view Message() const noexcept { return _message; }
        void SetMessage(string_view message) noexcept { _message = message; }
        void SetMessage(string&& message) noexcept { _message = std::move(message); }

        [[nodiscard]] optional<melonDS::MacAddress> MacAddress() const noexcept { return _macAddress; }
        void SetMacAddress(std::optional<melonDS::MacAddress> macAddress) noexcept { _macAddress = macAddress; }

        [[nodiscard]] optional<melonDS::IpAddress> DnsServer() const noexcept { return _dnsServer; }
        void SetDnsServer(optional<melonDS::IpAddress> dnsServer) noexcept { _dnsServer = dnsServer; }

#ifdef HAVE_JIT
        [[nodiscard]] bool JitEnable() const noexcept { return _jitEnable; }
        void SetJitEnable(bool enable) noexcept { _jitEnable = enable; }

        [[nodiscard]] unsigned MaxBlockSize() const noexcept { return _maxBlockSize; }
        void SetMaxBlockSize(unsigned maxBlockSize) noexcept { _maxBlockSize = maxBlockSize; }

        [[nodiscard]] bool LiteralOptimizations() const noexcept { return _literalOptimizations; }
        void SetLiteralOptimizations(bool enable) noexcept { _literalOptimizations = enable; }

        [[nodiscard]] bool BranchOptimizations() const noexcept { return _branchOptimizations; }
        void SetBranchOptimizations(bool enable) noexcept { _branchOptimizations = enable; }

#   ifdef HAVE_JIT_FASTMEM
        [[nodiscard]] bool FastMemory() const noexcept { return _fastMemory; }
        void SetFastMemory(bool enable) noexcept { _fastMemory = enable; }
#   endif
#endif

#ifdef HAVE_NETWORKING
        [[nodiscard]] MelonDsDs::NetworkMode NetworkMode() const noexcept { return _networkMode; }
        void SetNetworkMode(MelonDsDs::NetworkMode mode) noexcept { _networkMode = mode; }

#   ifdef HAVE_NETWORKING_DIRECT_MODE
        [[nodiscard]] string_view NetworkInterface() const noexcept { return _networkInterface; }
        void SetNetworkInterface(string_view networkInterface) noexcept { _networkInterface = networkInterface; }
        void SetNetworkInterface(string&& networkInterface) noexcept { _networkInterface = std::move(networkInterface); }
#   endif
#endif

#ifndef NDEBUG
        [[nodiscard]] bool ShowPointerCoordinates() const noexcept { return showPointerCoordinates; }
        void SetShowPointerCoordinates(bool show) noexcept { showPointerCoordinates = show; }
#endif

        [[nodiscard]] bool ShowUnsupportedFeatureWarnings() const noexcept { return showUnsupportedFeatureWarnings; }
        void SetShowUnsupportedFeatureWarnings(bool show) noexcept { showUnsupportedFeatureWarnings = show; }

        [[nodiscard]] bool ShowMicState() const noexcept { return showMicState; }
        void SetShowMicState(bool show) noexcept { showMicState = show; }

        [[nodiscard]] bool ShowCameraState() const noexcept { return showCameraState; }
        void SetShowCameraState(bool show) noexcept { showCameraState = show; }

        [[nodiscard]] bool ShowBiosWarnings() const noexcept { return showBiosWarnings; }
        void SetShowBiosWarnings(bool show) noexcept { showBiosWarnings = show; }

        [[nodiscard]] bool ShowCurrentLayout() const noexcept { return showCurrentLayout; }
        void SetShowCurrentLayout(bool show) noexcept { showCurrentLayout = show; }

        [[nodiscard]] bool ShowLidState() const noexcept { return showLidState; }
        void SetShowLidState(bool show) noexcept { showLidState = show; }

        [[nodiscard]] bool ShowBrightnessState() const noexcept { return showBrightnessState; }
        void SetShowBrightnessState(bool show) noexcept { showBrightnessState = show; }

        [[nodiscard]] bool DldiEnable() const noexcept { return _dldiEnable; }
        void SetDldiEnable(bool enable) noexcept { _dldiEnable = enable; }

        [[nodiscard]] bool DldiFolderSync() const noexcept { return _dldiFolderSync; }
        void SetDldiFolderSync(bool sync) noexcept { _dldiFolderSync = sync; }

        [[nodiscard]] string_view DldiFolderPath() const noexcept { return _dldiFolderPath; }
        void SetDldiFolderPath(string_view path) noexcept { _dldiFolderPath = path; }
        void SetDldiFolderPath(string&& path) noexcept { _dldiFolderPath = std::move(path); }

        [[nodiscard]] bool DldiReadOnly() const noexcept { return _dldiReadOnly; }
        void SetDldiReadOnly(bool readOnly) noexcept { _dldiReadOnly = readOnly; }

        [[nodiscard]] string_view DldiImagePath() const noexcept { return _dldiImagePath; }
        void SetDldiImagePath(string_view path) noexcept { _dldiImagePath = path; }
        void SetDldiImagePath(string&& path) noexcept { _dldiImagePath = std::move(path); }

        [[nodiscard]] unsigned DldiImageSize() const noexcept { return _dldiImageSize; }
        void SetDldiImageSize(unsigned size) noexcept { _dldiImageSize = size; }

        [[nodiscard]] bool DsiSdEnable() const noexcept { return _dsiSdEnable; }
        void SetDsiSdEnable(bool enable) noexcept { _dsiSdEnable = enable; }

        [[nodiscard]] bool DsiSdFolderSync() const noexcept { return _dsiSdFolderSync; }
        void SetDsiSdFolderSync(bool sync) noexcept { _dsiSdFolderSync = sync; }

        [[nodiscard]] string_view DsiSdFolderPath() const noexcept { return _dsiSdFolderPath; }
        void SetDsiSdFolderPath(string_view path) noexcept { _dsiSdFolderPath = path; }
        void SetDsiSdFolderPath(string&& path) noexcept { _dsiSdFolderPath = std::move(path); }

        [[nodiscard]] bool DsiSdReadOnly() const noexcept { return _dsiSdReadOnly; }
        void SetDsiSdReadOnly(bool readOnly) noexcept { _dsiSdReadOnly = readOnly; }

        [[nodiscard]] string_view DsiSdImagePath() const noexcept { return _dsiSdImagePath; }
        void SetDsiSdImagePath(string_view path) noexcept { _dsiSdImagePath = path; }
        void SetDsiSdImagePath(string&& path) noexcept { _dsiSdImagePath = std::move(path); }

        [[nodiscard]] unsigned DsiSdImageSize() const noexcept { return _dsiSdImageSize; }
        void SetDsiSdImageSize(unsigned size) noexcept { _dsiSdImageSize = size; }

        [[nodiscard]] unsigned FlushDelay() const noexcept { return _flushDelay; }
        void SetFlushDelay(unsigned delay) noexcept { _flushDelay = delay; }

        [[nodiscard]] unsigned NumberOfScreenLayouts() const noexcept { return _numberOfScreenLayouts; }
        void SetNumberOfScreenLayouts(unsigned numberOfScreenLayouts) noexcept { _numberOfScreenLayouts = numberOfScreenLayouts; }

        [[nodiscard]] std::span<const ScreenLayout> ScreenLayouts() const noexcept {
            return {_screenLayouts.data(), _numberOfScreenLayouts};
        }
        void SetScreenLayouts(const std::array<ScreenLayout, config::screen::MAX_SCREEN_LAYOUTS>& screenLayouts) noexcept { _screenLayouts = screenLayouts; }

        [[nodiscard]] unsigned ScreenGap() const noexcept { return _screenGap; }
        void SetScreenGap(unsigned screenGap) noexcept { _screenGap = screenGap; }

        [[nodiscard]] unsigned HybridRatio() const noexcept { return _hybridRatio; }
        void SetHybridRatio(unsigned hybridRatio) noexcept { _hybridRatio = hybridRatio; }

        [[nodiscard]] HybridSideScreenDisplay SmallScreenLayout() const noexcept { return _smallScreenLayout; }
        void SetSmallScreenLayout(HybridSideScreenDisplay smallScreenLayout) noexcept { _smallScreenLayout = smallScreenLayout; }

        [[nodiscard]] float CursorSize() const noexcept { return _cursorSize; }
        void SetCursorSize(float cursorSize) noexcept { _cursorSize = cursorSize; }

        [[nodiscard]] CursorMode CursorMode() const noexcept { return _cursorMode; }
        void SetCursorMode(MelonDsDs::CursorMode cursorMode) noexcept { _cursorMode = cursorMode; }

        [[nodiscard]] unsigned CursorTimeout() const noexcept { return _cursorTimeout; }
        void SetCursorTimeout(unsigned cursorTimeout) noexcept { _cursorTimeout = cursorTimeout; }

        [[nodiscard]] TouchMode TouchMode() const noexcept { return _touchMode; }
        void SetTouchMode(MelonDsDs::TouchMode touchMode) noexcept { _touchMode = touchMode; }

        [[nodiscard]] ConsoleType ConsoleType() const noexcept { return _consoleType; }
        void SetConsoleType(MelonDsDs::ConsoleType consoleType) noexcept { _consoleType = consoleType; }

        [[nodiscard]] BootMode BootMode() const noexcept { return _bootMode; }
        void SetBootMode(MelonDsDs::BootMode bootMode) noexcept { _bootMode = bootMode; }

        [[nodiscard]] SysfileMode SysfileMode() const noexcept { return _sysfileMode; }
        void SetSysfileMode(MelonDsDs::SysfileMode sysfileMode) noexcept { _sysfileMode = sysfileMode; }

        [[nodiscard]] unsigned DsPowerOkayThreshold() const noexcept { return _dsPowerOkayThreshold; }
        void SetDsPowerOkayThreshold(unsigned dsPowerOkayThreshold) noexcept { _dsPowerOkayThreshold = dsPowerOkayThreshold; }

        [[nodiscard]] unsigned PowerUpdateInterval() const noexcept { return _powerUpdateInterval; }
        void SetPowerUpdateInterval(unsigned powerUpdateInterval) noexcept { _powerUpdateInterval = powerUpdateInterval; }

        // TODO: Allow these paths to be customized
        string_view Bios9Path() const noexcept { return "bios9.bin"; }
        string_view Bios7Path() const noexcept { return "bios7.bin"; }
        string_view DsiBios9Path() const noexcept { return "dsi_bios9.bin"; }
        string_view DsiBios7Path() const noexcept { return "dsi_bios7.bin"; }

        [[nodiscard]] std::string_view GeneratedFirmwareSettingsPath() const noexcept {
            return "melonDS DS/wfcsettings.bin";
        }
        [[nodiscard]] string_view FirmwarePath(MelonDsDs::ConsoleType type) const noexcept {
            return type == ConsoleType::DSi ? DsiFirmwarePath() : FirmwarePath();
        }

        [[nodiscard]] string_view FirmwarePath() const noexcept { return _firmwarePath; }
        void SetFirmwarePath(string_view firmwarePath) noexcept { _firmwarePath = firmwarePath; }
        void SetFirmwarePath(string&& firmwarePath) noexcept { _firmwarePath = std::move(firmwarePath); }

        [[nodiscard]] string_view DsiFirmwarePath() const noexcept { return _dsiFirmwarePath; }
        void SetDsiFirmwarePath(string_view dsiFirmwarePath) noexcept { _dsiFirmwarePath = dsiFirmwarePath; }
        void SetDsiFirmwarePath(string&& dsiFirmwarePath) noexcept { _dsiFirmwarePath = std::move(dsiFirmwarePath); }

        [[nodiscard]] string_view DsiNandPath() const noexcept { return _dsiNandPath; }
        void SetDsiNandPath(string_view dsiNandPath) noexcept { _dsiNandPath = dsiNandPath; }
        void SetDsiNandPath(string&& dsiNandPath) noexcept { _dsiNandPath = std::move(dsiNandPath); }

        [[nodiscard]] int ScaleFactor() const noexcept { return _scaleFactor; }
        void SetScaleFactor(int scaleFactor) noexcept { _scaleFactor = scaleFactor; }

        [[nodiscard]] bool BetterPolygonSplitting() const noexcept { return _betterPolygonSplitting; }
        void SetBetterPolygonSplitting(bool betterPolygonSplitting) noexcept { _betterPolygonSplitting = betterPolygonSplitting; }

        [[nodiscard]] Renderer ConfiguredRenderer() const noexcept { return _configuredRenderer; }
        void SetConfiguredRenderer(Renderer configuredRenderer) noexcept { _configuredRenderer = configuredRenderer; }

        [[nodiscard]] bool ThreadedSoftRenderer() const noexcept { return _threadedSoftRenderer; }
        void SetThreadedSoftRenderer(bool threadedSoftRenderer) noexcept { _threadedSoftRenderer = threadedSoftRenderer; }

        [[nodiscard]] MelonDsDs::ScreenFilter ScreenFilter() const noexcept { return _screenFilter; }
        void SetScreenFilter(MelonDsDs::ScreenFilter screenFilter) noexcept { _screenFilter = screenFilter; }
    private:
        void CustomizeFirmware(melonDS::Firmware& firmware);
        MelonDsDs::MicButtonMode _micButtonMode = MelonDsDs::MicButtonMode::Hold;
        MelonDsDs::MicInputMode _micInputMode = *ParseMicInputMode(config::definitions::MicInput.default_value);
        melonDS::AudioBitDepth _bitDepth;
        melonDS::AudioInterpolation _interpolation;
        MelonDsDs::AlarmMode _alarmMode;
        optional<unsigned> _alarmHour;
        optional<unsigned> _alarmMinute;
        FirmwareLanguage _language;
        unsigned _birthdayMonth = 1;
        unsigned _birthdayDay = 1;
        Color _favoriteColor;
        MelonDsDs::UsernameMode _usernameMode;
        string _message;
        optional<melonDS::MacAddress> _macAddress;
        optional<melonDS::IpAddress> _dnsServer;
#ifdef JIT_ENABLED
        bool _jitEnable;
        unsigned _maxBlockSize;
        bool _literalOptimizations;
        bool _branchOptimizations;
#   ifdef HAVE_JIT_FASTMEM
        bool _fastMemory;
#   endif
#endif


#ifdef HAVE_NETWORKING
        MelonDsDs::NetworkMode _networkMode;
        bool _interfacesInitialized = false;
#   ifdef HAVE_NETWORKING_DIRECT_MODE
        string _networkInterface;
#   endif
#endif

#ifndef NDEBUG
        bool showPointerCoordinates = false;
#endif
        bool showUnsupportedFeatureWarnings = true;
        bool showMicState = true;
        bool showCameraState = true;
        bool showBiosWarnings = true;
        bool showCurrentLayout = true;
        bool showLidState = false;
        bool showBrightnessState = false;
        bool _dldiEnable;
        bool _dldiFolderSync;
        string _dldiFolderPath;
        bool _dldiReadOnly;
        string _dldiImagePath;
        unsigned _dldiImageSize;
        bool _dsiSdEnable;
        bool _dsiSdFolderSync;
        string _dsiSdFolderPath;
        bool _dsiSdReadOnly;
        string _dsiSdImagePath;
        unsigned _dsiSdImageSize;
        unsigned _flushDelay = 120; // TODO: Make configurable
        unsigned _numberOfScreenLayouts = 1;
        std::array<ScreenLayout, config::screen::MAX_SCREEN_LAYOUTS> _screenLayouts;
        unsigned _screenGap;
        unsigned _hybridRatio;
        HybridSideScreenDisplay _smallScreenLayout;
        unsigned _cursorSize = 2.0f;
        MelonDsDs::CursorMode _cursorMode = CursorMode::Always;
        unsigned _cursorTimeout;
        MelonDsDs::TouchMode _touchMode;
        MelonDsDs::ConsoleType _consoleType;
        MelonDsDs::BootMode _bootMode;
        MelonDsDs::SysfileMode _sysfileMode;
        unsigned _dsPowerOkayThreshold = 20;
        unsigned _powerUpdateInterval;
        string _firmwarePath;
        string _dsiFirmwarePath;
        string _dsiNandPath;
        int _scaleFactor = 1;
        bool _betterPolygonSplitting = false;
        Renderer _configuredRenderer;
        bool _threadedSoftRenderer = false;
        MelonDsDs::ScreenFilter _screenFilter;
    };
}

#endif //MELONDS_DS_CONFIG_HPP
