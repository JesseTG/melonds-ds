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
constexpr pntr_color BACKGROUND_COLOR_TOP = {.rgba = {.b = 0xBC, .g = 0xB7, .r = 0xFA, .a = 0xFF}}; // light pink
constexpr pntr_color TEXT_COLOR_TOP = {.rgba = {.b = 0x19, .g = 0x0F, .r = 0xD7, .a = 0xFF}}; // dark red
constexpr pntr_color BACKGROUND_COLOR_BOTTOM = {.rgba = {.b = 0x36, .g = 0x7D, .r = 0x63, .a = 0xFF}}; // dark green
constexpr pntr_color TEXT_COLOR_BOTTOM = {.rgba = {.b = 0x98, .g = 0xE5, .r = 0xE7, .a = 0xFF}}; // light green

static constexpr const char* const ERROR_TITLE = "Oh no! melonDS DS couldn't start...";
static constexpr const char* const SOLUTION_TITLE = "Here's what you can do:";
static constexpr const char* const THANK_YOU = "Thank you for using melonDS DS!";

using std::span;
using MelonDsDs::NDS_SCREEN_AREA;

// I intentionally fix the error message to the DS screen size to simplify the layout.
MelonDsDs::error::ErrorScreen::ErrorScreen(const config_exception& e, enum retro_language language) noexcept : exception(e) {
    ZoneScopedN(TracyFunction);

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
    ZoneScopedN(TracyFunction);
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
        translate(ERROR_TITLE),
        (NDS_SCREEN_WIDTH - titleTextSize.x) / 2,
        MARGIN,
        TEXT_COLOR_TOP
    );

    // finally, draw the error summary (wrapping lines as needed)
    pntr_draw_text_wrapped(
        topScreen,
        bodyFont,
        translate(exception.what()),
        MARGIN,
        titleTextSize.y + MARGIN * 2,
        NDS_SCREEN_WIDTH - MARGIN * 2,
        TEXT_COLOR_TOP
    );
}

void MelonDsDs::error::ErrorScreen::DrawBottomScreen(pntr_font* titleFont, pntr_font* bodyFont) const noexcept {
    ZoneScopedN(TracyFunction);
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
        translate(SOLUTION_TITLE),
        (NDS_SCREEN_WIDTH - titleTextSize.x) / 2,
        MARGIN,
        TEXT_COLOR_BOTTOM
    );

    // draw the solution details (wrapping lines as needed)
    pntr_draw_text_wrapped(
        bottomScreen,
        bodyFont,
        translate(exception.user_message()),
        MARGIN,
        titleTextSize.y + MARGIN * 2,
        NDS_SCREEN_WIDTH - MARGIN * 2,
        TEXT_COLOR_BOTTOM
    );

    pntr_vector thankYouTextSize = pntr_measure_text_ex(bodyFont, THANK_YOU, 0);
    pntr_draw_text(
        bottomScreen,
        bodyFont,
        translate(THANK_YOU),
        NDS_SCREEN_WIDTH - thankYouTextSize.x - MARGIN,
        NDS_SCREEN_HEIGHT - thankYouTextSize.y - MARGIN,
        TEXT_COLOR_BOTTOM
    );
}

span<const uint32_t, NDS_SCREEN_AREA<size_t>> MelonDsDs::error::ErrorScreen::TopScreen() const noexcept {
    return span<const uint32_t, NDS_SCREEN_AREA<size_t>>{(const uint32_t*)topScreen->data, NDS_SCREEN_AREA<size_t>};
}

span<const uint32_t, NDS_SCREEN_AREA<size_t>> MelonDsDs::error::ErrorScreen::BottomScreen() const noexcept {
    return span<const uint32_t, NDS_SCREEN_AREA<size_t>>{(const uint32_t*)bottomScreen->data, NDS_SCREEN_AREA<size_t>};
}

#include <stdio.h>
/**
 * Translates the given message into the active current `language`.
 * 
 * @param message The message to translate.
 * 
 * @return A translated message if a translation is available, otherwise the original message.
 */
const char* MelonDsDs::error::ErrorScreen::translate(const char* message) const noexcept {
    printf("Lagnuage: %d\n", (int)language);
    switch (language) {
        case RETRO_LANGUAGE_SPANISH:
            if (message == ERROR_TITLE)     return "¡Oh no! melonDS DS no pudo iniciar...";
            if (message == SOLUTION_TITLE)  return "Esto es lo que puedes hacer:";
            if (message == THANK_YOU)       return "¡Gracias por usar melonDS DS!";
        case RETRO_LANGUAGE_FRENCH:
            if (message == ERROR_TITLE)     return "Oh non! melonDS DS n'a pas pu démarrer...";
            if (message == SOLUTION_TITLE)  return "Voici ce que vous pouvez faire:";
            if (message == THANK_YOU)       return "Merci d'utiliser melonDS DS!";
        default:
            return message;
    }

    // TODO: Add translations of the possible exceptions?
}
