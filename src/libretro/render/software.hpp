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

#ifndef MELONDSDS_RENDER_SOFTWARE_HPP
#define MELONDSDS_RENDER_SOFTWARE_HPP

#include "render.hpp"

namespace MelonDsDs {
    class SoftwareRenderState final : public RenderState {
    public:
        bool Ready() const noexcept override { return true; }
        void Render(melonDS::NDS& nds, const InputState& input, const CoreConfig& config, ScreenLayoutData& screenLayout) noexcept override;
    };
}

#endif // MELONDSDS_RENDER_SOFTWARE_HPP
