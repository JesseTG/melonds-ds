/*
    Copyright 2024 Jesse Talavera

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

#pragma once

#include "std/chrono.hpp"

namespace retro::task {
    class TaskSpec;
}

namespace MelonDsDs {
    class CoreConfig;

    class RumbleState {
    public:
        [[nodiscard]] retro::task::TaskSpec RumbleTask() noexcept;
        void RumbleStart(std::chrono::milliseconds len) noexcept;
        void RumbleStop() noexcept;
    private:
        std::chrono::microseconds _rumbleTimeout;
    };
}