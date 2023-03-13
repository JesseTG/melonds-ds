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
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#ifndef MELONDS_DS_CONFIG_HPP
#define MELONDS_DS_CONFIG_HPP

#include <GPU.h>

namespace melonds {
    bool update_option_visibility();
    void check_variables(bool init);
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

    enum RendererType {
        Software,
        OpenGl,
    };

    GPU::RenderSettings &render_settings();
    extern ScreenSwapMode screen_swap_mode;
}

#endif //MELONDS_DS_CONFIG_HPP
