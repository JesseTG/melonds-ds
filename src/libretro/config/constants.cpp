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

#include "constants.hpp"

#include <string/stdstring.h>

using std::optional;
using std::nullopt;

optional<melonds::Renderer> melonds::config::ParseRenderer(const char* value) noexcept {
    if (string_is_equal(value, values::SOFTWARE)) return melonds::Renderer::Software;
    if (string_is_equal(value, values::OPENGL)) return melonds::Renderer::OpenGl;
    return nullopt;
}

optional<melonds::CursorMode> melonds::config::ParseCursorMode(const char* value) noexcept {
    if (string_is_equal(value, values::DISABLED)) return melonds::CursorMode::Never;
    if (string_is_equal(value, values::TOUCHING)) return melonds::CursorMode::Touching;
    if (string_is_equal(value, values::TIMEOUT)) return melonds::CursorMode::Timeout;
    if (string_is_equal(value, values::ALWAYS)) return melonds::CursorMode::Always;
    return nullopt;
}

optional<melonds::ConsoleType> melonds::config::ParseConsoleType(const char* value) noexcept {
    if (string_is_equal(value, values::DS)) return melonds::ConsoleType::DS;
    if (string_is_equal(value, values::DSI)) return melonds::ConsoleType::DSi;
    return nullopt;
}

optional<melonds::NetworkMode> melonds::config::ParseNetworkMode(const char* value) noexcept {
    if (string_is_equal(value, values::DISABLED)) return melonds::NetworkMode::None;
    if (string_is_equal(value, values::DIRECT)) return melonds::NetworkMode::Direct;
    if (string_is_equal(value, values::INDIRECT)) return melonds::NetworkMode::Indirect;
    return nullopt;
}

optional<bool> melonds::config::ParseBoolean(const char* value) noexcept {
    if (string_is_equal(value, values::ENABLED)) return true;
    if (string_is_equal(value, values::DISABLED)) return false;
    return nullopt;
}

optional<melonds::ScreenLayout> melonds::config::ParseScreenLayout(const char* value) noexcept {
    using melonds::ScreenLayout;
    if (string_is_equal(value, values::TOP_BOTTOM)) return ScreenLayout::TopBottom;
    if (string_is_equal(value, values::BOTTOM_TOP)) return ScreenLayout::BottomTop;
    if (string_is_equal(value, values::LEFT_RIGHT)) return ScreenLayout::LeftRight;
    if (string_is_equal(value, values::RIGHT_LEFT)) return ScreenLayout::RightLeft;
    if (string_is_equal(value, values::TOP)) return ScreenLayout::TopOnly;
    if (string_is_equal(value, values::BOTTOM)) return ScreenLayout::BottomOnly;
    if (string_is_equal(value, values::HYBRID_TOP)) return ScreenLayout::HybridTop;
    if (string_is_equal(value, values::HYBRID_BOTTOM)) return ScreenLayout::HybridBottom;
    if (string_is_equal(value, values::ROTATE_LEFT)) return ScreenLayout::TurnLeft;
    if (string_is_equal(value, values::ROTATE_RIGHT)) return ScreenLayout::TurnRight;
    if (string_is_equal(value, values::UPSIDE_DOWN)) return ScreenLayout::UpsideDown;

    return nullopt;
}

optional<melonds::HybridSideScreenDisplay> melonds::config::ParseHybridSideScreenDisplay(const char* value) noexcept {
    using melonds::ScreenLayout;
    if (string_is_equal(value, values::ONE)) return melonds::HybridSideScreenDisplay::One;
    if (string_is_equal(value, values::BOTH)) return melonds::HybridSideScreenDisplay::Both;

    return nullopt;
}
