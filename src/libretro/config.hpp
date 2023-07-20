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

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include <glsym/glsym.h>
#endif
#include <GPU.h>
#include <libretro.h>
#include <NDS_Header.h>

// TODO: Move everything into melonds::config
namespace melonds {
    class ScreenLayoutData;
    class InputState;

    /// Called when loading a game
    void InitConfig(
        const std::optional<retro_game_info>& nds_info,
        const std::optional<NDSHeader>& header,
        ScreenLayoutData& screenLayout,
        InputState& inputState
    );

    /// Called when settings have been updated mid-game
    void UpdateConfig(ScreenLayoutData& screenLayout, InputState& inputState) noexcept;
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

    using MacAddress = std::array<std::uint8_t, 6>;

    namespace config {
        namespace audio {
            [[nodiscard]] BitDepth BitDepth() noexcept;
            [[nodiscard]] AudioInterpolation Interpolation() noexcept;

            [[nodiscard]] MicButtonMode MicButtonMode() noexcept;
            [[nodiscard]] MicInputMode MicInputMode() noexcept;
        }

        namespace firmware {
            [[nodiscard]] bool FirmwareSettingsOverrideEnable() noexcept;
            [[nodiscard]] FirmwareLanguage Language() noexcept;
            [[nodiscard]] unsigned BirthdayMonth() noexcept;
            [[nodiscard]] unsigned BirthdayDay() noexcept;
            [[nodiscard]] Color FavoriteColor() noexcept;
            [[nodiscard]] std::string Username() noexcept;
            [[nodiscard]] std::string Message() noexcept;
            [[nodiscard]] MacAddress MacAddress() noexcept;
        }

        namespace jit {
            [[nodiscard]] unsigned MaxBlockSize() noexcept;
            [[nodiscard]] bool Enable() noexcept;
            [[nodiscard]] bool LiteralOptimizations() noexcept;
            [[nodiscard]] bool BranchOptimizations() noexcept;
            [[nodiscard]] bool FastMemory() noexcept;
        }

        namespace system {
            [[nodiscard]] ConsoleType ConsoleType() noexcept;
            [[nodiscard]] bool DirectBoot() noexcept;
            [[nodiscard]] bool ExternalBiosEnable() noexcept;
            [[nodiscard]] unsigned DsPowerOkayThreshold() noexcept;
            [[nodiscard]] unsigned PowerUpdateInterval() noexcept;
            [[nodiscard]] std::string Bios9Path() noexcept;
            [[nodiscard]] std::string Bios7Path() noexcept;
            [[nodiscard]] std::string FirmwarePath() noexcept;
            [[nodiscard]] std::string DsiBios9Path() noexcept;
            [[nodiscard]] std::string DsiBios7Path() noexcept;
            [[nodiscard]] std::string DsiFirmwarePath() noexcept;
            [[nodiscard]] std::string DsiNandPath() noexcept;

            [[nodiscard]] bool RandomizeMac() noexcept;
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
        }

        namespace video {
            constexpr unsigned INITIAL_MAX_OPENGL_SCALE = 4;
            constexpr unsigned MAX_OPENGL_SCALE = 8;
            [[nodiscard]] Renderer ConfiguredRenderer() noexcept;
            [[nodiscard]] GPU::RenderSettings RenderSettings() noexcept;
            [[nodiscard]] ScreenFilter ScreenFilter() noexcept;
            [[nodiscard]] int ScaleFactor() noexcept;
        }
    }
}

#endif //MELONDS_DS_CONFIG_HPP
