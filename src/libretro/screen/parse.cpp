/**
    Copyright 2025 Jesse Talavera

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

#include "parse.hpp"

#include <variant>

#include <tinyexpr.h>
#include <toml.hpp>

#include "format.hpp"

using namespace MelonDsDs::Layout;

using std::optional;
using std::string;
using std::vector;

using toml::error_info;
using toml::make_error_info;

template<>
struct toml::from<ParsedExpression> {
    template<typename TC>
    static ParsedExpression from_toml(const basic_value<TC>& val) {
        switch (val.type()) {
            case value_t::integer:
                return static_cast<int>(val.as_integer());
            case value_t::floating:
                return static_cast<float>(val.as_floating());
            case value_t::string:
                return val.as_string();
            default:
                throw type_error(fmt::format("Expected integer, floating-point, or string for Expression; got {}", val.type()), val.location());
        }
        __builtin_unreachable();
        return -1; // Unreachable, but avoids compiler warnings
    }
};

template<>
struct toml::from<retro::ScreenOrientation> {
    template<typename TC>
    static retro::ScreenOrientation from_toml(const basic_value<TC>& val) {
        if (!val.is_string())
            throw type_error(fmt::format("Expected a string for ScreenOrientation; got {}", val.type()), val.location());

        std::string_view orientation = toml::get<std::string_view, TC>(val);
        if (orientation == "none") return retro::ScreenOrientation::Normal;
        if (orientation == "left") return retro::ScreenOrientation::RotatedLeft;
        if (orientation == "right") return retro::ScreenOrientation::RotatedRight;
        if (orientation == "upsidedown") return retro::ScreenOrientation::UpsideDown;

        throw type_error(fmt::format("Unknown ScreenOrientation value '{}' (expected 'none', 'left', 'right', or 'upsidedown')", orientation), val.location());
    }
};

template<>
struct toml::from<MelonDsDs::ScreenFilter>
{
    template<typename TC>
    static MelonDsDs::ScreenFilter from_toml(const basic_value<TC>& val) {
        if (!val.is_string())
            throw type_error(fmt::format("Expected a string for ScreenFilter; got {}", val.type()), val.location());

        std::string_view filter = toml::get<std::string_view, TC>(val);
        if (filter == "linear") return MelonDsDs::ScreenFilter::Linear;
        if (filter == "nearest") return MelonDsDs::ScreenFilter::Nearest;

        throw type_error(fmt::format("Unknown ScreenFilter value '{}' (expected 'linear' or 'nearest')", filter), val.location());
    }
};

template<>
struct toml::from<ScreenType> {
    template<typename TC>
    static ScreenType from_toml(const basic_value<TC>& val) {
        if (!val.is_string())
            throw type_error(fmt::format("Expected a string for ScreenType; got {}", val.type()), val.location());

        std::string_view type = toml::get<std::string_view, TC>(val);
        if (type == "top") return ScreenType::Top;
        if (type == "touch") return ScreenType::Touch;
        if (type == "3d") return ScreenType::ThreeD;

        throw type_error(fmt::format("Unknown ScreenType value '{}' (expected 'top', 'touch', or '3d')", type), val.location());
    }
};

template<>
struct toml::from<ParsedVector> {
    template<typename TC>
    static ParsedVector from_toml(const basic_value<TC>& val) {
        if (!val.is_table())
            throw type_error(fmt::format("Expected a table for ParsedVector; got {}", val.type()), val.location());

        ParsedVector vector;
        vector.x = toml::find<ParsedExpression, TC>(val, "x");
        vector.y = toml::find<ParsedExpression, TC>(val, "y");
        return vector;
    }
};

template<>
struct toml::from<ParsedLayout> {
    template<typename TC>
    static ParsedLayout from_toml(const basic_value<TC>& val) {
        if (!val.is_table())
            throw type_error(fmt::format("Expected a table for ParsedLayout; got {}", val.type()), val.location());

        ParsedLayout layout;
        layout.name = toml::find<optional<string>, TC>(val, "name");
        layout.orientation = toml::find<optional<retro::ScreenOrientation>, TC>(val, "orientation");
        layout.screens = toml::find<vector<ParsedScreen>, TC>(val, "screens");
        return layout;
    }
};

template<>
struct toml::from<ParsedScreen> {
    template<typename TC>
    static ParsedScreen from_toml(const basic_value<TC>& val) {
        if (!val.is_table())
            throw type_error(fmt::format("Expected a table for ParsedScreen; got {}", val.type()), val.location());

        ParsedScreen screen;
        screen.type = toml::find<ScreenType, TC>(val, "type");
        screen.position = toml::find<ParsedVector, TC>(val, "position");

        if (val.contains("scale")) {
            // If this screen explicitly defines a scale...
            switch (const basic_value<TC>& scaleValue = val.at("scale"); scaleValue.type()) {
                case value_t::integer: {
                    auto scale = scaleValue.as_integer();
                    screen.scale = {scale, scale};
                    break;
                }
                case value_t::floating: {
                    auto scale = scaleValue.as_floating();
                    screen.scale = ParsedVector {scale, scale};
                    break;
                }
                case value_t::table: {
                    screen.scale = toml::get<ParsedVector, TC>(scaleValue);
                    break;
                }
                default: {
                    throw type_error(fmt::format("Expected integer, floating-point, or table for scale; got {}", scaleValue.type()), scaleValue.location());
                }
            }
        }
        // Otherwise use the default-constructed value of {1, 1}

        screen.visible = toml::find<optional<ParsedExpression>>(val, "if");
        screen.filter = toml::find<optional<MelonDsDs::ScreenFilter>, TC>(val, "filter");
        return screen;
    }
};

ParsedLayoutConfig::ParsedLayoutConfig(const string& toml) noexcept : ParsedLayoutConfig(string(toml)) {
}

ParsedLayoutConfig::ParsedLayoutConfig(std::string&& toml) noexcept {
    // We want to use order-preserving maps so that
    // user-defined layouts are displayed in the same order they're defined.
    auto result = toml::try_parse_str<toml::ordered_type_config>(std::move(toml));

    if (result.is_err()) {
        errors = std::move(result.as_err());
        return;
    }

    const toml::ordered_value& root = result.as_ok();
    if (!root.is_table()) {
        error_info info = make_error_info("Expected a root table", root, fmt::format("But got: {}", root.type()));
        errors.push_back(std::move(info));
        return;
    }

    const toml::ordered_table& rootTable = root.as_table();
    for (const auto& [key, value]: rootTable) {
        try {
            ParsedLayout layout = toml::get<ParsedLayout, toml::ordered_type_config>(value);
            layouts[key] = std::move(layout);
        } catch (const toml::type_error& e) {
            // Too bad toml11 has no good way to return errors in user-defined conversions
            error_info info = make_error_info(fmt::format("Error parsing layout '{}'", key), e.location(), e.what());
            errors.push_back(std::move(info));
        } catch (const std::out_of_range& e) {
            error_info info = make_error_info(fmt::format("Out of range error parsing layout '{}'", key), value.location(), e.what());
            errors.push_back(std::move(info));
        }
    }
}

ParsedLayoutConfig::operator bool() const noexcept {
    return !layouts.empty();
}
