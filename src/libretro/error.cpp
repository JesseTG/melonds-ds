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

#include "screenlayout.hpp"
#include <cstring>
#include <nuklear.h>

constexpr float ERROR_TEXT_HEIGHT = 14.0f;
constexpr unsigned FONT_BITMAP_WIDTH = 1024;
constexpr unsigned FONT_BITMAP_HEIGHT = 1024;

melonds::error::ErrorScreen::ErrorScreen(const config_exception& e) noexcept : exception(e) {
    fontConfig = nk_font_config(ERROR_TEXT_HEIGHT);
    nk_font_atlas_init_default(&fonts);
    font = nk_font_atlas_add_default(&fonts, ERROR_TEXT_HEIGHT, &fontConfig);
    userFont.userdata.ptr = font;
    userFont.height = ERROR_TEXT_HEIGHT;
    bool ctx_initialized = nk_init_default(&ctx, &userFont);
    assert(ctx_initialized);
}

melonds::error::ErrorScreen::~ErrorScreen() {
    nk_font_atlas_clear(&fonts);
    nk_free(&ctx);
}

void melonds::error::ErrorScreen::Render(ScreenLayoutData& screenLayout) noexcept {
    if (screenLayout.Dirty()) {
        screenLayout.Update(melonds::Renderer::Software);
    }

    screenLayout.Clear();

    retro::video_refresh(
        screenLayout.Buffer()[0],
        screenLayout.Buffer().Width(),
        screenLayout.Buffer().Height(),
        screenLayout.Buffer().Stride()
    );
}
