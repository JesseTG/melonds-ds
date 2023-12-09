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

#ifndef MELONDS_DS_RENDER_HPP
#define MELONDS_DS_RENDER_HPP

namespace melonDS {
    class NDS;
}

namespace MelonDsDs {
    class InputState;
    class ScreenLayoutData;
    class CoreConfig;

    class RenderState {
    public:
        virtual ~RenderState() noexcept = default;

        /// Returns true if all state necessary for rendering is ready.
        /// This includes the OpenGL context (if applicable) and the emulator's renderer.
        virtual bool Ready() const noexcept = 0;
        virtual void Render(const melonDS::NDS& nds, const InputState& input, const CoreConfig& config, const ScreenLayoutData& screenLayout) noexcept = 0;
        virtual void RequestRefresh() {}
    };
}

#endif //MELONDS_DS_RENDER_HPP
