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

#include "cheats.hpp"

#include <charconv>
#include <memory>
#include <regex>
#include <ARCodeFile.h>
#include <string/stdstring.h>
#include <retro_assert.h>

#include "libretro.hpp"
#include "environment.hpp"
#include "tracy.hpp"

using std::csub_match;
using std::unique_ptr;
using std::regex;
using std::make_unique;
using std::from_chars_result;
using std::cregex_iterator;
using namespace std::regex_constants;

namespace AREngine {
    extern void RunCheat(ARCode &arcode);
}

static unique_ptr<regex> cheatSyntax;
static unique_ptr<regex> tokenSyntax;

void melonds::cheats::deinit() noexcept {
    cheatSyntax = nullptr;
    tokenSyntax = nullptr;
}

PUBLIC_SYMBOL void retro_cheat_reset(void) {
    retro::debug("retro_cheat_reset()");
}

PUBLIC_SYMBOL void retro_cheat_set(unsigned index, bool enabled, const char *code) {
    // Cheat codes are small programs, so we can't exactly turn them off (that would be undoing them)
    ZoneScopedN("retro_cheat_set");
    retro::debug("retro_cheat_set({}, {}, {})\n", index, enabled, code);
    if (string_is_empty(code))
        return;

    if (!enabled) {
        retro::set_warn_message("Action Replay codes can't be undone, please restart the game to remove their effects.");
        return;
    }

    if (!cheatSyntax) {
        cheatSyntax = make_unique<regex>("^\\s*[0-9A-Fa-f]{8}([+\\s]*[0-9A-Fa-f]{8})*$", ECMAScript | optimize);
    }

    if (!tokenSyntax) {
        tokenSyntax = make_unique<regex>("[0-9A-Fa-f]{8}", ECMAScript | optimize);
    }

    if (enabled && !regex_match(code, *cheatSyntax)) {
        // If we're trying to activate this cheat code, but it's not valid...
        retro::set_warn_message("Cheat #{} ({:.8}...) isn't valid, ignoring it.", index, code);
        return;
    }

    melonDS::ARCode curcode {
        .Name = "",
        .Enabled = enabled,
        .Code = {}
    };

    // NDS cheats are sequence of unsigned 32-bit integers, each of which is hex-encoded
    size_t length = strlen(code);
    auto end = cregex_iterator();
    for (auto i = cregex_iterator(code, code + length, *tokenSyntax); i != end; ++i)
    {
        const csub_match& match = (*i)[0];
        retro_assert(match.matched);
        uint32_t token = 0;
        from_chars_result result = std::from_chars(match.first, match.second, token, 16);
        retro_assert(result.ec == std::errc());
        curcode.Code.push_back(token);
    }

    AREngine::RunCheat(curcode);
}