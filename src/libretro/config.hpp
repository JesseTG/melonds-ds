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

#include <optional>

#ifdef HAVE_OPENGL
#include <glsym/glsym.h>
#endif
#include <GPU.h>
#include <libretro.h>
#include <NDS_Header.h>

// TODO: Move everything into melonds::config
namespace melonds {
    /// Called when loading a game
    void InitConfig(const std::optional<struct retro_game_info>& nds_info, const std::optional<NDSHeader>& header);

    /// Called when settings have been updated mid-game
    void UpdateConfig(const std::optional<struct retro_game_info>& nds_info, const std::optional<NDSHeader>& header) noexcept;
    bool update_option_visibility();
    extern struct retro_core_options_v2 options_us;
    extern struct retro_core_option_v2_definition option_defs_us[];
#ifndef HAVE_NO_LANGEXTRA
    extern struct retro_core_options_v2* options_intl[];
#endif

    enum class ConsoleType {
        DS = 0,
        DSi = 1,
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

    enum class Renderer {
        None = -1,
        Software = 0, // To match with values that melonDS expects
        OpenGl = 1,
    };

    enum class BitDepth {
        Auto = 0,
        _10Bit = 1,
        _16Bit = 2,
    };

    enum class AudioInterpolation {
        None = 0,
        Linear = 1,
        Cosine = 2,
        Cubic = 3,
    };

    enum class MicInputMode {
        None,
        HostMic,
        WhiteNoise,
        BlowNoise,
    };

    enum class FirmwareLanguage {
        Japanese = 0,
        English = 1,
        French = 2,
        German = 3,
        Italian = 4,
        Spanish = 5,
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

    enum class SdCardMode {
        None,
        Shared,
        Dedicated,
    };

    enum class SmallScreenLayout {
        SmallScreenTop = 0,
        SmallScreenBottom = 1,
        SmallScreenDuplicate = 2
    };

    enum class ScreenId {
        Primary = 0,
        Top = 1,
        Bottom = 2,
    };

    enum class TouchMode {
        Disabled,
        Mouse,
        Touch,
        Joystick,
    };

    using MacAddress = std::array<std::uint8_t, 6>;

    namespace config {
        namespace audio {
            BitDepth BitDepth() noexcept;
            AudioInterpolation Interpolation() noexcept;

            MicButtonMode MicButtonMode() noexcept;
            MicInputMode MicInputMode() noexcept;
        }

        namespace firmware {
            bool FirmwareSettingsOverrideEnable() noexcept;
            FirmwareLanguage Language() noexcept;
            unsigned BirthdayMonth() noexcept;
            unsigned BirthdayDay() noexcept;
            Color FavoriteColor() noexcept;
            std::string Username() noexcept;
            std::string Message() noexcept;
            MacAddress MacAddress() noexcept;
        }

        namespace jit {
            unsigned MaxBlockSize() noexcept;
            bool Enable() noexcept;
            bool LiteralOptimizations() noexcept;
            bool BranchOptimizations() noexcept;
            bool FastMemory() noexcept;
        }

        namespace system {
            ConsoleType ConsoleType() noexcept;
            bool DirectBoot() noexcept;
            bool ExternalBiosEnable() noexcept;
            std::string Bios9Path() noexcept;
            std::string Bios7Path() noexcept;
            std::string FirmwarePath() noexcept;
            std::string DsiBios9Path() noexcept;
            std::string DsiBios7Path() noexcept;
            std::string DsiFirmwarePath() noexcept;
            std::string DsiNandPath() noexcept;

            bool RandomizeMac() noexcept;
        }

        namespace save {
            SdCardMode DldiSdCardMode() noexcept;
            bool DldiEnable() noexcept;
            bool DldiFolderSync() noexcept;
            std::string DldiFolderPath() noexcept;
            bool DldiReadOnly() noexcept;
            std::string DldiImagePath() noexcept;
            unsigned DldiImageSize() noexcept;

            SdCardMode DsiSdCardMode() noexcept;
            bool DsiSdEnable() noexcept;
            bool DsiSdFolderSync() noexcept;
            std::string DsiSdFolderPath() noexcept;
            bool DsiSdReadOnly() noexcept;
            std::string DsiSdImagePath() noexcept;
            unsigned DsiSdImageSize() noexcept;

            unsigned FlushDelay() noexcept;
        }

        namespace screen {
            ScreenLayout ScreenLayout() noexcept;
            unsigned ScreenGap() noexcept;
            unsigned HybridRatio() noexcept;
            ScreenSwapMode ScreenSwapMode() noexcept;
            SmallScreenLayout SmallScreenLayout() noexcept;
            TouchMode TouchMode() noexcept;
        }

        namespace video {
            float CursorSize() noexcept;
            Renderer ConfiguredRenderer() noexcept;
            GPU::RenderSettings RenderSettings() noexcept;
            ScreenFilter ScreenFilter() noexcept;
            int ScaleFactor() noexcept;
        }
    }
}

#endif //MELONDS_DS_CONFIG_HPP
