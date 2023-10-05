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

#include "error.hpp"
#include "buffer.hpp"

#include "embedded/melondsds_graphic_error.h"
#include "embedded/melondsds_graphic_sorry.h"
#include "screenlayout.hpp"
#include <pntr.h>

constexpr float ERROR_TEXT_HEIGHT = 14.0f;
constexpr pntr_color BACKGROUND_COLOR_TOP =  { .b = 0xD2, .g = 0xCF, .r = 0xFC, .a = 0xFF }; // light pink
constexpr pntr_color BACKGROUND_COLOR_BOTTOM =  { .b = 0x36, .g = 0x7D, .r = 0x63, .a = 0xFF }; // dark green

melonds::error::ErrorScreen::ErrorScreen(const config_exception& e) noexcept : exception(e) {
    topScreen = pntr_gen_image_color(NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT, BACKGROUND_COLOR_TOP);
    assert(topScreen != nullptr);

    bottomScreen = pntr_gen_image_color(NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT, BACKGROUND_COLOR_BOTTOM);
    assert(bottomScreen != nullptr);

    pntr_image* errorIcon = pntr_load_image_from_memory(PNTR_IMAGE_TYPE_PNG, embedded_melondsds_graphic_error, sizeof(embedded_melondsds_graphic_error));
    assert(errorIcon != nullptr);

    pntr_image* sorryIcon = pntr_load_image_from_memory(PNTR_IMAGE_TYPE_PNG, embedded_melondsds_graphic_sorry, sizeof(embedded_melondsds_graphic_sorry));
    assert(sorryIcon != nullptr);

    pntr_font* font = pntr_load_font_default();
    assert(font != nullptr);

    pntr_draw_image(bottomScreen, sorryIcon, NDS_SCREEN_WIDTH / 2, NDS_SCREEN_HEIGHT / 2);
    pntr_draw_image(topScreen, errorIcon, NDS_SCREEN_WIDTH / 2, NDS_SCREEN_HEIGHT / 2);

    pntr_unload_font(font);
    pntr_unload_image(errorIcon);
    pntr_unload_image(sorryIcon);
}

melonds::error::ErrorScreen::~ErrorScreen() {
    if (topScreen) {
        pntr_unload_image(topScreen);
    }

    if (bottomScreen) {
        pntr_unload_image(bottomScreen);
    }
}

void melonds::error::ErrorScreen::Render(ScreenLayoutData& screenLayout) noexcept {
    if (screenLayout.Dirty()) {
        screenLayout.Update(melonds::Renderer::Software);
    }

    screenLayout.Clear();
    screenLayout.CombineScreens(
        reinterpret_cast<const uint32_t*>(topScreen->data),
        reinterpret_cast<const uint32_t*>(bottomScreen->data)
    );
    // TODO: Start rendering with pntr

    retro::video_refresh(
        screenLayout.Buffer()[0],
        screenLayout.Buffer().Width(),
        screenLayout.Buffer().Height(),
        screenLayout.Buffer().Stride()
    );
}
