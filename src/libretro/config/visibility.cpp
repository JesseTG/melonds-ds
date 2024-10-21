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

#include "visibility.hpp"

#include <optional>

#include "config/parse.hpp"
#include "environment.hpp"
#include "screenlayout.hpp"
#include "tracy.hpp"

using std::optional;

bool MelonDsDs::CoreOptionVisibility::Update() noexcept {
    ZoneScopedN(TracyFunction);
    using namespace MelonDsDs::config;
    using retro::environment;
    using retro::get_variable;
    using retro::set_option_visible;
    bool updated = false;

    retro::debug(TracyFunction);

    // Convention: if an option is not found, show any dependent options
    bool oldShowMicButtonMode = ShowMicButtonMode;
    optional<MicInputMode> micInputMode = ParseMicInputMode(get_variable(audio::MIC_INPUT));
    ShowMicButtonMode = !micInputMode || *micInputMode != MicInputMode::None;
    if (!VisibilityInitialized || ShowMicButtonMode != oldShowMicButtonMode) {
        set_option_visible(audio::MIC_INPUT_BUTTON, ShowMicButtonMode);
        updated = true;
    }

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
    // Show/hide OpenGL core options
    bool oldShowOpenGlOptions = ShowOpenGlOptions;
    bool oldShowSoftwareRenderOptions = ShowSoftwareRenderOptions;
    optional<RenderMode> renderer = ParseRenderMode(get_variable(video::RENDER_MODE));
    ShowOpenGlOptions = !renderer || *renderer == RenderMode::OpenGl;
    ShowSoftwareRenderOptions = !ShowOpenGlOptions;
    if (!VisibilityInitialized || ShowOpenGlOptions != oldShowOpenGlOptions) {
        set_option_visible(video::OPENGL_RESOLUTION, ShowOpenGlOptions);
        set_option_visible(video::OPENGL_FILTERING, ShowOpenGlOptions);
        set_option_visible(video::OPENGL_BETTER_POLYGONS, ShowOpenGlOptions);
        updated = true;
    }
#ifdef HAVE_THREADED_RENDERER
    if (!VisibilityInitialized || ShowSoftwareRenderOptions != oldShowSoftwareRenderOptions) {
        set_option_visible(video::THREADED_RENDERER, ShowSoftwareRenderOptions);
        updated = true;
    }
#endif

#else
    set_option_visible(video::RENDER_MODE, false);
#endif

    bool oldShowDsiOptions = ShowDsiOptions;
    optional<ConsoleType> consoleType = ParseConsoleType(get_variable(system::CONSOLE_MODE));
    ShowDsiOptions = !consoleType || *consoleType == ConsoleType::DSi;
    if (!VisibilityInitialized || ShowDsiOptions != oldShowDsiOptions) {
        set_option_visible(config::system::FIRMWARE_DSI_PATH, ShowDsiOptions);
        set_option_visible(config::storage::DSI_NAND_PATH, ShowDsiOptions);
        set_option_visible(storage::DSI_SD_SAVE_MODE, ShowDsiOptions);
        updated = true;
    }

    bool oldShowDsiSdCardOptions = ShowDsiSdCardOptions && ShowDsiOptions;
    optional<bool> dsiSdEnable = ParseBoolean(get_variable(storage::DSI_SD_SAVE_MODE));
    ShowDsiSdCardOptions = !dsiSdEnable || *dsiSdEnable;
    if (!VisibilityInitialized || ShowDsiSdCardOptions != oldShowDsiSdCardOptions) {
        set_option_visible(storage::DSI_SD_READ_ONLY, ShowDsiSdCardOptions);
        set_option_visible(storage::DSI_SD_SYNC_TO_HOST, ShowDsiSdCardOptions);
        updated = true;
    }

    bool oldShowDsOptions = ShowDsOptions;
    ShowDsOptions = !consoleType || *consoleType == ConsoleType::DS;
    if (!VisibilityInitialized || ShowDsOptions != oldShowDsOptions) {
        set_option_visible(config::system::SYSFILE_MODE, ShowDsOptions);
        set_option_visible(config::system::FIRMWARE_PATH, ShowDsOptions);
        set_option_visible(config::system::DS_POWER_OK, ShowDsOptions);
        set_option_visible(system::SLOT2_DEVICE, ShowDsOptions);
        updated = true;
    }

    bool oldShowHomebrewSdOptions = ShowHomebrewSdOptions;
    optional<bool> homebrewSdCardEnabled = ParseBoolean(get_variable(storage::HOMEBREW_SAVE_MODE));
    ShowHomebrewSdOptions = !homebrewSdCardEnabled || *homebrewSdCardEnabled;
    if (!VisibilityInitialized || ShowHomebrewSdOptions != oldShowHomebrewSdOptions) {
        set_option_visible(storage::HOMEBREW_READ_ONLY, ShowHomebrewSdOptions);
        set_option_visible(storage::HOMEBREW_SYNC_TO_HOST, ShowHomebrewSdOptions);
        updated = true;
    }

    bool oldShowCursorTimeout = ShowCursorTimeout;
    optional<CursorMode> cursorMode = ParseCursorMode(get_variable(screen::SHOW_CURSOR));
    ShowCursorTimeout = !cursorMode || *cursorMode == CursorMode::Timeout;
    if (!VisibilityInitialized || ShowCursorTimeout != oldShowCursorTimeout) {
        set_option_visible(screen::CURSOR_TIMEOUT, ShowCursorTimeout);
        updated = true;
    }

    unsigned oldNumberOfShownScreenLayouts = NumberOfShownScreenLayouts;
    optional<unsigned> numberOfScreenLayouts = ParseIntegerInRange(get_variable(screen::NUMBER_OF_SCREEN_LAYOUTS), 1u, screen::MAX_SCREEN_LAYOUTS);
    NumberOfShownScreenLayouts = numberOfScreenLayouts ? *numberOfScreenLayouts : screen::MAX_SCREEN_LAYOUTS;
    if (!VisibilityInitialized || NumberOfShownScreenLayouts != oldNumberOfShownScreenLayouts) {
        for (unsigned i = 0; i < screen::MAX_SCREEN_LAYOUTS; ++i) {
            set_option_visible(screen::SCREEN_LAYOUTS[i], i < NumberOfShownScreenLayouts);
        }
        updated = true;
    }

    // Show/hide Hybrid screen options
    bool oldShowHybridOptions = ShowHybridOptions;
    bool oldShowVerticalLayoutOptions = ShowVerticalLayoutOptions;
    bool anyHybridLayouts = false;
    bool anyVerticalLayouts = false;
    for (unsigned i = 0; i < NumberOfShownScreenLayouts; i++) {
        optional<MelonDsDs::ScreenLayout> parsedLayout = ParseScreenLayout(get_variable(screen::SCREEN_LAYOUTS[i]));
        anyHybridLayouts |= !parsedLayout || IsHybridLayout(*parsedLayout);
        anyVerticalLayouts |= !parsedLayout || LayoutSupportsScreenGap(*parsedLayout);
    }
    ShowHybridOptions = anyHybridLayouts;
    ShowVerticalLayoutOptions = anyVerticalLayouts;

    if (!VisibilityInitialized || ShowHybridOptions != oldShowHybridOptions) {
        set_option_visible(screen::HYBRID_SMALL_SCREEN, ShowHybridOptions);
        set_option_visible(screen::HYBRID_RATIO, ShowHybridOptions);
        updated = true;
    }

    if (!VisibilityInitialized || ShowVerticalLayoutOptions != oldShowVerticalLayoutOptions) {
        set_option_visible(screen::SCREEN_GAP, ShowVerticalLayoutOptions);
        updated = true;
    }

    bool oldShowAlarm = ShowAlarm;
    optional<AlarmMode> alarmMode = ParseAlarmMode(get_variable(firmware::ENABLE_ALARM));
    ShowAlarm = !alarmMode || *alarmMode == AlarmMode::Enabled;
    if (!VisibilityInitialized || ShowAlarm != oldShowAlarm) {
        set_option_visible(firmware::ALARM_HOUR, ShowAlarm);
        set_option_visible(firmware::ALARM_MINUTE, ShowAlarm);
        updated = true;
    }

#ifdef JIT_ENABLED
    // Show/hide JIT core options
    bool oldShowJitOptions = ShowJitOptions;
    optional<bool> jitEnabled = MelonDsDs::ParseBoolean(get_variable(cpu::JIT_ENABLE));
    ShowJitOptions = !jitEnabled || *jitEnabled;
    if (!VisibilityInitialized || ShowJitOptions != oldShowJitOptions) {
        set_option_visible(cpu::JIT_BLOCK_SIZE, ShowJitOptions);
        set_option_visible(cpu::JIT_BRANCH_OPTIMISATIONS, ShowJitOptions);
        set_option_visible(cpu::JIT_LITERAL_OPTIMISATIONS, ShowJitOptions);
#ifdef HAVE_JIT_FASTMEM
        set_option_visible(cpu::JIT_FAST_MEMORY, ShowJitOptions);
#endif
        updated = true;
    }
#endif

#ifdef HAVE_NETWORKING_DIRECT_MODE
    bool oldShowWifiInterface = ShowWifiInterface;
    optional<NetworkMode> networkMode = ParseNetworkMode(get_variable(network::NETWORK_MODE));

    ShowWifiInterface = !networkMode || *networkMode == NetworkMode::Direct;
    if (!VisibilityInitialized || ShowWifiInterface != oldShowWifiInterface) {
        set_option_visible(network::DIRECT_NETWORK_INTERFACE, ShowWifiInterface);
        updated = true;
    }
#endif

    optional<StartTimeMode> timeMode = ParseStartTimeMode(get_variable(time::START_TIME_MODE));
    bool oldShowRelativeTime = ShowRelativeStartTime;
    ShowRelativeStartTime = !timeMode || *timeMode == StartTimeMode::Relative;
    if (!VisibilityInitialized || ShowRelativeStartTime != oldShowRelativeTime) {
        set_option_visible(time::RELATIVE_YEAR_OFFSET, ShowRelativeStartTime);
        set_option_visible(time::RELATIVE_DAY_OFFSET, ShowRelativeStartTime);
        set_option_visible(time::RELATIVE_HOUR_OFFSET, ShowRelativeStartTime);
        set_option_visible(time::RELATIVE_MINUTE_OFFSET, ShowRelativeStartTime);
        updated = true;
    }

    bool oldShowAbsoluteTime = ShowAbsoluteStartTime;
    ShowAbsoluteStartTime = !timeMode || *timeMode == StartTimeMode::Absolute;
    if (!VisibilityInitialized || ShowAbsoluteStartTime != oldShowAbsoluteTime) {
        set_option_visible(time::ABSOLUTE_YEAR, ShowAbsoluteStartTime);
        set_option_visible(time::ABSOLUTE_MONTH, ShowAbsoluteStartTime);
        set_option_visible(time::ABSOLUTE_DAY, ShowAbsoluteStartTime);
        set_option_visible(time::ABSOLUTE_HOUR, ShowAbsoluteStartTime);
        set_option_visible(time::ABSOLUTE_MINUTE, ShowAbsoluteStartTime);
        updated = true;
    }

    VisibilityInitialized = true;
    return updated;
}
