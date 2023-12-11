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

#ifndef MELONDSDS_CONFIG_CONSOLE_HPP
#define MELONDSDS_CONFIG_CONSOLE_HPP

#include <memory>

namespace melonDS {
    class NDS;
}

namespace retro {
    class GameInfo;
}

namespace MelonDsDs {
    class CoreConfig;
    std::unique_ptr<melonDS::NDS> CreateConsole(
        const CoreConfig& config,
        const retro::GameInfo* ndsInfo,
        const retro::GameInfo* gbaInfo,
        const retro::GameInfo* gbaSaveInfo
    );

    void UpdateConsole(const CoreConfig& config, melonDS::NDS& nds);

    void ResetConsole(const CoreConfig& config, melonDS::NDS& nds);
}

#endif // MELONDSDS_CONFIG_CONSOLE_HPP
