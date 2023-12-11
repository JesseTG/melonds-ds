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

#ifndef MELONDSDS_CONFIG_VISIBILITY_HPP
#define MELONDSDS_CONFIG_VISIBILITY_HPP

#include "constants.hpp"

namespace MelonDsDs {
    struct CoreOptionVisibility {
        bool Update() noexcept;
        bool ShowMicButtonMode = true;
        bool ShowHomebrewSdOptions = true;
        bool ShowDsOptions = true;
        bool ShowDsiOptions = true;
        bool ShowDsiSdCardOptions = true;
        bool ShowSoftwareRenderOptions = true;
        bool ShowHybridOptions = true;
        bool ShowVerticalLayoutOptions = true;
        bool ShowCursorTimeout = true;
        bool ShowAlarm = true;
        unsigned NumberOfShownScreenLayouts = config::screen::MAX_SCREEN_LAYOUTS;
#ifdef JIT_ENABLED
        bool ShowJitOptions = true;
#endif
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
        bool ShowOpenGlOptions = true;
#endif
#ifdef HAVE_NETWORKING_DIRECT_MODE
        bool ShowWifiInterface = true;
#endif
    };
}
#endif // MELONDSDS_CONFIG_VISIBILITY_HPP
