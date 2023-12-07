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

#include <pntr.h>

#include "buffer.hpp"
#include "embedded/melondsds_graphic_error.h"
#include "embedded/melondsds_graphic_sorry.h"
#include "embedded/melondsds_error_title_font.h"
#include "embedded/melondsds_error_body_font.h"
#include "screenlayout.hpp"
#include "tracy.hpp"

constexpr int TITLE_FONT_HEIGHT = 20; // in pixels
constexpr int BODY_FONT_HEIGHT = 18; // in pixels
constexpr int MARGIN = 8; // in pixels
constexpr pntr_color BACKGROUND_COLOR_TOP = {.b = 0xBC, .g = 0xB7, .r = 0xFA, .a = 0xFF}; // light pink
constexpr pntr_color TEXT_COLOR_TOP = {.b = 0x19, .g = 0x0F, .r = 0xD7, .a = 0xFF}; // dark red
constexpr pntr_color BACKGROUND_COLOR_BOTTOM = {.b = 0x36, .g = 0x7D, .r = 0x63, .a = 0xFF}; // dark green
constexpr pntr_color TEXT_COLOR_BOTTOM = {.b = 0x98, .g = 0xE5, .r = 0xE7, .a = 0xFF}; // light green

static constexpr const char* const ERROR_TITLE = "Oh no! melonDS DS couldn't start...";
static constexpr const char* const SOLUTION_TITLE = "Here's what you can do:";
static constexpr const char* const THANK_YOU = "Thank you for using melonDS DS!";

// I intentionally fix the error message to the DS screen size to simplify the layout.
MelonDsDs::error::ErrorScreen::ErrorScreen(const config_exception& e) noexcept : exception(e) {
    ZoneScopedN("MelonDsDs::error::ErrorScreen::ErrorScreen");

    pntr_font* titleFont = pntr_load_font_ttf_from_memory(
        embedded_melondsds_error_title_font,
        sizeof(embedded_melondsds_error_title_font),
        TITLE_FONT_HEIGHT
    );
    assert(titleFont != nullptr);

    pntr_font* bodyFont = pntr_load_font_ttf_from_memory(
        embedded_melondsds_error_body_font,
        sizeof(embedded_melondsds_error_body_font),
        BODY_FONT_HEIGHT
    );
    assert(bodyFont != nullptr);

    topScreen = pntr_gen_image_color(NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT, BACKGROUND_COLOR_TOP);
    assert(topScreen != nullptr);

    bottomScreen = pntr_gen_image_color(NDS_SCREEN_WIDTH, NDS_SCREEN_HEIGHT, BACKGROUND_COLOR_BOTTOM);
    assert(bottomScreen != nullptr);

    // Y coordinates go down, and the origin for all images is in their top-left corner.
    DrawTopScreen(titleFont, bodyFont);
    DrawBottomScreen(titleFont, bodyFont);

    pntr_unload_font(titleFont);
    pntr_unload_font(bodyFont);
}

MelonDsDs::error::ErrorScreen::~ErrorScreen() {
    pntr_unload_image(topScreen);
    pntr_unload_image(bottomScreen);
}

void MelonDsDs::error::ErrorScreen::DrawTopScreen(pntr_font* titleFont, pntr_font* bodyFont) const noexcept {
    ZoneScopedN("MelonDsDs::error::ErrorScreen::DrawTopScreen");
    assert(titleFont != nullptr);

    pntr_image* errorIcon = pntr_load_image_from_memory(
        PNTR_IMAGE_TYPE_PNG,
        embedded_melondsds_graphic_error,
        sizeof(embedded_melondsds_graphic_error)
    );
    assert(errorIcon != nullptr);
    assert(errorIcon->height < NDS_SCREEN_HEIGHT);
    assert(errorIcon->width < NDS_SCREEN_WIDTH);

    // draw a little watermelon emoji in the bottom-right corner
    pntr_draw_image(
        topScreen,
        errorIcon,
        NDS_SCREEN_WIDTH - errorIcon->width - MARGIN,
        NDS_SCREEN_HEIGHT - errorIcon->height - MARGIN
    );
    pntr_unload_image(errorIcon);

    // now draw the title
    pntr_vector titleTextSize = pntr_measure_text_ex(titleFont, ERROR_TITLE, 0);
    pntr_draw_text(
        topScreen,
        titleFont,
        ERROR_TITLE,
        (NDS_SCREEN_WIDTH - titleTextSize.x) / 2,
        MARGIN,
        TEXT_COLOR_TOP
    );

    // finally, draw the error summary (wrapping lines as needed)
    pntr_draw_text_wrapped(
        topScreen,
        bodyFont,
        exception.what(),
        MARGIN,
        titleTextSize.y + MARGIN * 2,
        NDS_SCREEN_WIDTH - MARGIN * 2,
        TEXT_COLOR_TOP
    );
}

void MelonDsDs::error::ErrorScreen::DrawBottomScreen(pntr_font* titleFont, pntr_font* bodyFont) const noexcept {
    ZoneScopedN("MelonDsDs::error::ErrorScreen::DrawBottomScreen");
    assert(titleFont != nullptr);

    pntr_image* sorryIcon = pntr_load_image_from_memory(
        PNTR_IMAGE_TYPE_PNG,
        embedded_melondsds_graphic_sorry,
        sizeof(embedded_melondsds_graphic_sorry)
    );
    assert(sorryIcon != nullptr);
    assert(sorryIcon->height < NDS_SCREEN_HEIGHT);
    assert(sorryIcon->width < NDS_SCREEN_WIDTH);

    // draw a little watermelon emoji in the bottom-left corner
    pntr_draw_image(
        bottomScreen,
        sorryIcon,
        MARGIN,
        NDS_SCREEN_HEIGHT - sorryIcon->height - MARGIN
    );
    pntr_unload_image(sorryIcon);

    // now draw the title
    pntr_vector titleTextSize = pntr_measure_text_ex(titleFont, SOLUTION_TITLE, 0);
    pntr_draw_text(
        bottomScreen,
        titleFont,
        SOLUTION_TITLE,
        (NDS_SCREEN_WIDTH - titleTextSize.x) / 2,
        MARGIN,
        TEXT_COLOR_BOTTOM
    );

    // draw the solution details (wrapping lines as needed)
    pntr_draw_text_wrapped(
        bottomScreen,
        bodyFont,
        exception.user_message(),
        MARGIN,
        titleTextSize.y + MARGIN * 2,
        NDS_SCREEN_WIDTH - MARGIN * 2,
        TEXT_COLOR_BOTTOM
    );

    pntr_vector thankYouTextSize = pntr_measure_text_ex(bodyFont, THANK_YOU, 0);
    pntr_draw_text(
        bottomScreen,
        bodyFont,
        THANK_YOU,
        NDS_SCREEN_WIDTH - thankYouTextSize.x - MARGIN,
        NDS_SCREEN_HEIGHT - thankYouTextSize.y - MARGIN,
        TEXT_COLOR_BOTTOM
    );
}

void MelonDsDs::error::ErrorScreen::Render(ScreenLayoutData& screenLayout) const noexcept {
    ZoneScopedN("MelonDsDs::error::ErrorScreen::Render");
    if (screenLayout.Dirty()) {
        screenLayout.Update(MelonDsDs::Renderer::Software);
    }

    screenLayout.Clear();
    screenLayout.CombineScreens(
        reinterpret_cast<const uint32_t*>(topScreen->data),
        reinterpret_cast<const uint32_t*>(bottomScreen->data)
    );

    retro::video_refresh(
        screenLayout.Buffer()[0],
        screenLayout.Buffer().Width(),
        screenLayout.Buffer().Height(),
        screenLayout.Buffer().Stride()
    );
}
