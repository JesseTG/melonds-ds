/*
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

#include "parser.hpp"

#include <tao/pegtl.hpp>

// Defines PEGTL rules for the VFL grammar described at https://tinyurl.com/5n9afy7t
namespace MelonDsDs::Vfl::Grammar {
    namespace pegtl = TAO_PEGTL_NAMESPACE;
    using namespace pegtl;

    // arbitrarily long whitespace
    struct ws : star<space> {
    };

    // decimal number with optional sign and decimal point (no scientific notation, we don't need it)
    struct number : seq<
            opt<one<'+', '-'>>, // optional "+" or "-"
            plus<digit>, // one or more digits
            opt<seq<one<'.'>, star<digit>>> // optional decimal point followed by zero or more digits
        > {
    };

    // 'H' or 'V' for horizontal or vertical orientation
    struct orientation : one<'H', 'V'> {
    };

    // start of the visual format string
    struct orientation_prefix : seq<orientation, one<':'>> {
    };

    struct superview : one<'|'> {
    };

    // relation operators: "==", "<=", ">="
    struct relation : sor<
            string<'=', '='>,
            string<'<', '='>,
            string<'>', '='>
        > {
    };

    struct metric_name : identifier {
    };

    struct view_name : identifier {
    };

    struct priority : sor<metric_name, number> {
    };

    struct constant : sor<metric_name, number> {
    };

    struct object_of_predicate : sor<constant, view_name> {
    };

    // Forward declarations
    struct predicate;
    struct predicate_list_with_parens;

    struct simple_predicate : sor<metric_name, number> {
    };

    struct predicate_list : pegtl::sor<
            simple_predicate,
            predicate_list_with_parens
        > {
    };

    struct priority_suffix : seq<
            one<'@'>,
            priority
        > {
    };

    struct predicate : seq<
            opt<relation>,
            object_of_predicate,
            opt<priority_suffix>
        > {
    };

    // comma-separated list of predicates, enclosed in parentheses
    struct predicate_list_with_parens : seq<
            one<'('>,
            ws,
            predicate,
            ws,
            star<seq<
                one<','>,
                ws,
                predicate,
                ws
            >>,
            one<')'>
        > {
    };

    struct view : seq<
            one<'['>,
            view_name,
            opt<predicate_list_with_parens>,
            one<']'>
        > {
    };

    struct connection_with_predicate : seq<
            one<'-'>,
            predicate_list,
            one<'-'>
        > {
    };

    struct connection : sor<seq<>, connection_with_predicate, one<'-'>> {
    };

    struct view_connection : seq<
            connection,
            view
        > {
    };

    struct visual_format_string : seq<
            ws,
            opt<orientation_prefix>,
            ws,
            opt<seq<superview, connection>>,
            view,
            star<view_connection>,
            opt<seq<connection, superview>>,
            ws
        > {
    };

    // Main grammar rule
    struct grammar : must<visual_format_string, eof> {
    };
}
