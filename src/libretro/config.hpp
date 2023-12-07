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

namespace melonDS {
    struct NDSArgs;
    struct DSiArgs;
    struct NDSHeader;
    struct RenderSettings;
    class NDS;
}

namespace melondsds {
    struct CoreState;
}

struct retro_core_options_v2;
struct retro_core_option_v2_definition;
struct retro_game_info;

// TODO: Move everything into melonds::config
namespace melonds {
    class ScreenLayoutData;
    class InputState;

    /// Called when loading a game
    [[deprecated("Split into LoadConfig and ApplyConfig")]] void InitConfig(
        melondsds::CoreState& core,
        const melonDS::NDSHeader* header, // I'd like to have an optional<NDSHeader&>, but C++ doesn't allow it
        ScreenLayoutData& screenLayout,
        InputState& inputState
    );

    /// Called when settings have been updated mid-game
    void UpdateConfig(melondsds::CoreState& core, ScreenLayoutData& screenLayout, InputState& inputState) noexcept;
    bool update_option_visibility();

    enum class ConsoleType {
        DS = 0,
        DSi = 1,
    };

    enum class BiosType {
        Arm7,
        Arm9,
        Arm7i,
        Arm9i,
    };

    enum class ScreenSwapMode {
        Hold,
        Toggle,
    };

    enum class MicButtonMode {
        Hold,
        Toggle,
        Always,
    };

    enum class TouchMode {
        Auto,
        Pointer,
        Joystick,
    };

    enum class Renderer {
        None = -1,
        Software = 0, // To match with values that melonDS expects
        OpenGl = 1,
    };

    enum class MicInputMode {
        None,
        HostMic,
        WhiteNoise,
    };

    enum class FirmwareLanguage {
        Japanese = 0,
        English = 1,
        French = 2,
        German = 3,
        Italian = 4,
        Spanish = 5,
        Chinese = 6,
        Default = 7,
        Auto = 8,
    };

    enum class AlarmMode {
        Default,
        Enabled,
        Disabled,
    };

    enum class BootMode {
        Direct,
        Native,
    };

    enum class SysfileMode {
        BuiltIn,
        Native,
    };

    enum class UsernameMode {
        MelonDSDS,
        Guess,
        Firmware,
    };

    enum class Color {
        Gray,
        Brown,
        Red,
        LightPink,
        Orange,
        Yellow,
        Lime,
        LightGreen,
        DarkGreen,
        Turquoise,
        LightBlue,
        Blue,
        DarkBlue,
        DarkPurple,
        LightPurple,
        DarkPink,
        Default,
    };

    enum class ScreenFilter {
        Nearest,
        Linear,
    };


    enum class ScreenLayout {
        TopBottom = 0,
        BottomTop = 1,
        LeftRight = 2,
        RightLeft = 3,
        TopOnly = 4,
        BottomOnly = 5,
        HybridTop = 6,
        HybridBottom = 7,
        TurnLeft = 8,
        TurnRight = 9,
        UpsideDown = 10,
    };

    enum class HybridSideScreenDisplay {
        One,
        Both
    };

    enum class CursorMode {
        Never,
        Touching,
        Timeout,
        Always,
    };

    enum class NetworkMode {
        None,
        Direct,
        Indirect,
    };

    namespace config {
        namespace audio {
            [[nodiscard]] melonDS::AudioBitDepth BitDepth() noexcept;
            [[nodiscard]] melonDS::AudioInterpolation Interpolation() noexcept;

            [[nodiscard]] MicButtonMode MicButtonMode() noexcept;
            [[nodiscard]] MicInputMode MicInputMode() noexcept;
        }

        namespace jit {
            [[nodiscard]] unsigned MaxBlockSize() noexcept;
            [[nodiscard]] bool Enable() noexcept;
            [[nodiscard]] bool LiteralOptimizations() noexcept;
            [[nodiscard]] bool BranchOptimizations() noexcept;
            [[nodiscard]] bool FastMemory() noexcept;
        }

        namespace net {
#ifdef HAVE_NETWORKING
            [[nodiscard]] NetworkMode NetworkMode() noexcept;
#else
            [[nodiscard]] NetworkMode NetworkMode() noexcept { return NetworkMode::None; }
#endif

#ifdef HAVE_NETWORKING_DIRECT_MODE
            [[nodiscard]] std::string_view NetworkInterface() noexcept;
#else
            [[nodiscard]] constexpr std::string_view NetworkInterface() noexcept { return ""; }
#endif
        }

        namespace osd {
            #ifndef NDEBUG
            [[nodiscard]] bool ShowPointerCoordinates() noexcept;
            #else
            [[nodiscard]] constexpr bool ShowPointerCoordinates() noexcept { return false; }
            #endif
            [[nodiscard]] bool ShowUnsupportedFeatureWarnings() noexcept;
            [[nodiscard]] bool ShowMicState() noexcept;
            [[nodiscard]] bool ShowCameraState() noexcept;
            [[nodiscard]] bool ShowBiosWarnings() noexcept;
            [[nodiscard]] bool ShowCurrentLayout() noexcept;
            [[nodiscard]] bool ShowLidState() noexcept;
            [[nodiscard]] bool ShowBrightnessState() noexcept;
        }

        namespace system {
            [[nodiscard]] ConsoleType ConsoleType() noexcept;
            [[nodiscard]] bool DirectBoot() noexcept;
            [[nodiscard]] bool ExternalBiosEnable() noexcept;
            [[nodiscard]] unsigned DsPowerOkayThreshold() noexcept;
            [[nodiscard]] unsigned PowerUpdateInterval() noexcept;
            [[nodiscard]] std::string_view Bios9Path() noexcept;
            [[nodiscard]] std::string_view Bios7Path() noexcept;
            [[nodiscard]] std::string_view FirmwarePath() noexcept;
            [[nodiscard]] std::string_view DsiBios9Path() noexcept;
            [[nodiscard]] std::string_view DsiBios7Path() noexcept;
            [[nodiscard]] std::string_view DsiFirmwarePath() noexcept;
            [[nodiscard]] std::string_view DsiNandPath() noexcept;
            [[nodiscard]] std::string_view GeneratedFirmwareSettingsPath() noexcept;
            [[nodiscard]] inline std::string_view FirmwarePath(enum ConsoleType type) noexcept {
                return type == ConsoleType::DSi ? DsiFirmwarePath() : FirmwarePath();
            }
        }

        namespace save {
            [[nodiscard]] bool DldiEnable() noexcept;
            [[nodiscard]] bool DldiFolderSync() noexcept;
            [[nodiscard]] std::string DldiFolderPath() noexcept;
            [[nodiscard]] bool DldiReadOnly() noexcept;
            [[nodiscard]] std::string DldiImagePath() noexcept;
            [[nodiscard]] unsigned DldiImageSize() noexcept;

            [[nodiscard]] bool DsiSdEnable() noexcept;
            [[nodiscard]] bool DsiSdFolderSync() noexcept;
            [[nodiscard]] std::string DsiSdFolderPath() noexcept;
            [[nodiscard]] bool DsiSdReadOnly() noexcept;
            [[nodiscard]] std::string DsiSdImagePath() noexcept;
            [[nodiscard]] unsigned DsiSdImageSize() noexcept;

            [[nodiscard]] unsigned FlushDelay() noexcept;
        }

        namespace screen {
            constexpr unsigned MAX_HYBRID_RATIO = 3;
            constexpr unsigned MAX_SCREEN_LAYOUTS = 8; // Chosen arbitrarily; if you need more, open a PR
            constexpr unsigned MAX_SCREEN_GAP = 128;
            [[nodiscard]] unsigned NumberOfScreenLayouts() noexcept;
            [[nodiscard]] std::array<ScreenLayout, MAX_SCREEN_LAYOUTS> ScreenLayouts() noexcept;
            [[nodiscard]] unsigned ScreenGap() noexcept;
            [[nodiscard]] unsigned HybridRatio() noexcept;
            [[nodiscard]] HybridSideScreenDisplay SmallScreenLayout() noexcept;
            [[nodiscard]] float CursorSize() noexcept;
            [[nodiscard]] CursorMode CursorMode() noexcept;
            [[nodiscard]] unsigned CursorTimeout() noexcept;
            [[nodiscard]] enum TouchMode TouchMode() noexcept;
        }

        namespace video {
            constexpr unsigned INITIAL_MAX_OPENGL_SCALE = 4;
            constexpr unsigned MAX_OPENGL_SCALE = 8;
            [[nodiscard]] Renderer ConfiguredRenderer() noexcept;
            [[nodiscard]] ScreenFilter ScreenFilter() noexcept;
#ifdef HAVE_THREADED_RENDERER
            [[nodiscard]] bool ThreadedSoftRenderer() noexcept;
#else
            [[nodiscard]] constexpr bool ThreadedSoftRenderer() noexcept { return false; }
#endif
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
            [[nodiscard]] int ScaleFactor() noexcept;
            [[nodiscard]] bool BetterPolygonSplitting() noexcept;
#else
            [[nodiscard]] constexpr int ScaleFactor() noexcept { return 1; }
            [[nodiscard]] constexpr bool BetterPolygonSplitting() noexcept { return false; }
#endif
        }
    }
}

namespace MelonDsDs {
    using namespace melonds;
    using std::string;
    using std::string_view;
    using std::optional;
    using namespace std::chrono;

    class Config {
    public:
        [[nodiscard]] MicButtonMode MicButtonMode() const noexcept { return _micButtonMode; }
        void SetMicButtonMode(melonds::MicButtonMode mode) noexcept { _micButtonMode = mode; }

        [[nodiscard]] MicInputMode MicInputMode() const noexcept { return _micInputMode; }
        void SetMicInputMode(melonds::MicInputMode mode) noexcept { _micInputMode = mode; }

        [[nodiscard]] melonDS::AudioBitDepth BitDepth() const noexcept { return _bitDepth; }
        void SetBitDepth(melonDS::AudioBitDepth bitDepth) noexcept { _bitDepth = bitDepth; }

        [[nodiscard]] melonDS::AudioInterpolation Interpolation() const noexcept { return _interpolation; }
        void SetInterpolation(melonDS::AudioInterpolation interpolation) noexcept { _interpolation = interpolation; }

        [[nodiscard]] AlarmMode AlarmMode() const noexcept { return _alarmMode; }
        void SetAlarmMode(melonds::AlarmMode alarmMode) noexcept { _alarmMode = alarmMode; }

        [[nodiscard]] optional<unsigned> AlarmHour() const noexcept { return _alarmHour; }
        void SetAlarmHour(optional<unsigned> hour) noexcept { _alarmHour = hour; }

        [[nodiscard]] optional<unsigned> AlarmMinute() const noexcept { return _alarmMinute; }
        void SetAlarmMinute(optional<unsigned> minute) noexcept { _alarmMinute = minute; }

        [[nodiscard]] optional<hh_mm_ss<minutes>> Alarm() const noexcept {
            if (!_alarmHour.has_value() || !_alarmMinute.has_value()) {
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

        [[nodiscard]] month_day Birthday() const noexcept {
            return month(_birthdayMonth) / day(_birthdayDay);
        }

        [[nodiscard]] Color FavoriteColor() const noexcept { return _favoriteColor; }
        void SetFavoriteColor(Color color) noexcept { _favoriteColor = color; }

        [[nodiscard]] melonds::UsernameMode UsernameMode() const noexcept { return _usernameMode; }
        void SetUsernameMode(melonds::UsernameMode mode) noexcept { _usernameMode = mode; }

        [[nodiscard]] string_view Message() const noexcept { return _message; }
        void SetMessage(string_view message) noexcept { _message = message; }
        void SetMessage(string&& message) noexcept { _message = std::move(message); }

        [[nodiscard]] optional<melonDS::MacAddress> MacAddress() const noexcept { return _macAddress; }
        void SetMacAddress(std::optional<melonDS::MacAddress> macAddress) noexcept { _macAddress = macAddress; }

        [[nodiscard]] optional<melonDS::IpAddress> DnsServer() const noexcept { return _dnsServer; }
        void SetDnsServer(melonDS::IpAddress dnsServer) noexcept { _dnsServer = std::move(dnsServer); }

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
        [[nodiscard]] melonds::NetworkMode NetworkMode() const noexcept { return _networkMode; }
        void SetNetworkMode(melonds::NetworkMode mode) noexcept { _networkMode = mode; }

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
        void SetCursorMode(melonds::CursorMode cursorMode) noexcept { _cursorMode = cursorMode; }

        [[nodiscard]] unsigned CursorTimeout() const noexcept { return _cursorTimeout; }
        void SetCursorTimeout(unsigned cursorTimeout) noexcept { _cursorTimeout = cursorTimeout; }

        [[nodiscard]] TouchMode TouchMode() const noexcept { return _touchMode; }
        void SetTouchMode(melonds::TouchMode touchMode) noexcept { _touchMode = touchMode; }

        [[nodiscard]] ConsoleType ConsoleType() const noexcept { return _consoleType; }
        void SetConsoleType(melonds::ConsoleType consoleType) noexcept { _consoleType = consoleType; }

        [[nodiscard]] BootMode BootMode() const noexcept { return _bootMode; }
        void SetBootMode(melonds::BootMode bootMode) noexcept { _bootMode = bootMode; }

        [[nodiscard]] SysfileMode SysfileMode() const noexcept { return _sysfileMode; }
        void SetSysfileMode(melonds::SysfileMode sysfileMode) noexcept { _sysfileMode = sysfileMode; }

        [[nodiscard]] unsigned DsPowerOkayThreshold() const noexcept { return _dsPowerOkayThreshold; }
        void SetDsPowerOkayThreshold(unsigned dsPowerOkayThreshold) noexcept { _dsPowerOkayThreshold = dsPowerOkayThreshold; }

        [[nodiscard]] unsigned PowerUpdateInterval() const noexcept { return _powerUpdateInterval; }
        void SetPowerUpdateInterval(unsigned powerUpdateInterval) noexcept { _powerUpdateInterval = powerUpdateInterval; }

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

        [[nodiscard]] melonds::ScreenFilter ScreenFilter() const noexcept { return _screenFilter; }
        void SetScreenFilter(melonds::ScreenFilter screenFilter) noexcept { _screenFilter = screenFilter; }
    private:
        melonds::MicButtonMode _micButtonMode = melonds::MicButtonMode::Hold;
        melonds::MicInputMode _micInputMode;
        melonDS::AudioBitDepth _bitDepth;
        melonDS::AudioInterpolation _interpolation;
        melonds::AlarmMode _alarmMode;
        optional<unsigned> _alarmHour;
        optional<unsigned> _alarmMinute;
        FirmwareLanguage _language;
        unsigned _birthdayMonth = 1;
        unsigned _birthdayDay = 1;
        Color _favoriteColor;
        melonds::UsernameMode _usernameMode;
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
        melonds::NetworkMode _networkMode;
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
        melonds::CursorMode _cursorMode = CursorMode::Always;
        unsigned _cursorTimeout;
        melonds::TouchMode _touchMode;
        melonds::ConsoleType _consoleType;
        melonds::BootMode _bootMode;
        melonds::SysfileMode _sysfileMode;
        unsigned _dsPowerOkayThreshold = 20;
        unsigned _powerUpdateInterval;
        string _firmwarePath;
        string _dsiFirmwarePath;
        string _dsiNandPath;
        int _scaleFactor = 1;
        bool _betterPolygonSplitting = false;
        Renderer _configuredRenderer;
        bool _threadedSoftRenderer = false;
        melonds::ScreenFilter _screenFilter;
    };
}

#endif //MELONDS_DS_CONFIG_HPP
