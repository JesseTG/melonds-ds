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

#ifdef HAVE_OPENGL
#include <glsym/glsym.h>
#endif
#include <GPU.h>
#include <libretro.h>

namespace melonds {
    bool update_option_visibility();
    void update_variables(bool init) noexcept;
    void apply_variables(bool init) noexcept;
    extern struct retro_core_options_v2 options_us;
    extern struct retro_core_option_v2_definition option_defs_us[];
#ifndef HAVE_NO_LANGEXTRA
    extern struct retro_core_options_v2 *options_intl[];
#endif

    enum ConsoleType {
        DS = 0,
        DSi = 1,
    };

    enum class ScreenSwapMode {
        Hold,
        Toggle,
    };

    enum class MicButtonMode {
        Hold,
        //Toggle,
        Always,
    };

    enum class Renderer
    {
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

    enum class MicInputMode
    {
        None,
        HostMic,
        WhiteNoise,
        BlowNoise,
    };

    enum class FirmwareLanguage
    {
        Japanese = 0,
        English = 1,
        French = 2,
        German = 3,
        Italian = 4,
        Spanish = 5,
    };
}

namespace Config::Retro {
    extern melonds::MicButtonMode MicButtonMode;
    extern bool RandomizeMac;
    extern float CursorSize;
    extern melonds::ScreenSwapMode ScreenSwapMode;
    extern melonds::Renderer CurrentRenderer;
    extern melonds::Renderer ConfiguredRenderer;

    // The number of frames to wait for the save data buffer to not change before saving.
    extern int FlushDelay;

    GPU::RenderSettings RenderSettings();
}

#endif //MELONDS_DS_CONFIG_HPP
