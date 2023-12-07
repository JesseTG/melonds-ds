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

#ifndef MELONDS_DS_ERROR_HPP
#define MELONDS_DS_ERROR_HPP

#include "exceptions.hpp"

struct pntr_font;
struct pntr_image;

namespace MelonDsDs {
    class ScreenLayoutData;
    class PixelBuffer;
}

namespace MelonDsDs::error {
    class ErrorScreen {
    public:
        explicit ErrorScreen(const config_exception& e) noexcept;
        ~ErrorScreen();
        ErrorScreen(const ErrorScreen&) = delete;
        ErrorScreen& operator=(const ErrorScreen&) = delete;
        ErrorScreen(ErrorScreen&&) = delete;
        ErrorScreen& operator=(ErrorScreen&&) = delete;

        void Render(ScreenLayoutData& screenLayout) const noexcept;
    private:
        void DrawTopScreen(pntr_font* titleFont, pntr_font* bodyFont) const noexcept;
        void DrawBottomScreen(pntr_font* titleFont, pntr_font* bodyFont) const noexcept;
        config_exception exception;
        pntr_image* bottomScreen = nullptr;
        pntr_image* topScreen = nullptr;
    };
}

#endif //MELONDS_DS_ERROR_HPP
