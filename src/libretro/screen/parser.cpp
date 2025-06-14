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

#include <cctype>
#include <charconv>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <retro_assert.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include "environment.hpp"
#include "tracy/client.hpp"

using std::map;
using std::string;
using std::variant;
using std::false_type;
using std::true_type;
using std::string_view;

// Defines PEGTL rules for the VFL grammar described at https://tinyurl.com/5n9afy7t
namespace MelonDsDs::Vfl {
    namespace pegtl = TAO_PEGTL_NAMESPACE;
    using namespace pegtl;

    // arbitrarily long whitespace
    struct ws : star<space> {
    };

    // decimal number with optional sign and decimal point (no scientific notation, we don't need it)
    struct number : seq<
            opt<one<'-'>>, // optional "-"
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

    // Defining separate rules for leading and trailing superviews
    // simplifies constructing the AST, even if they're both identical
    struct leading_superview : one<'|'> {
    };

    struct trailing_superview : one<'|'> {
    };

    // relation operators: "==", "<=", ">="
    struct relation : sor<
            pegtl::string<'=', '='>,
            pegtl::string<'<', '='>,
            pegtl::string<'>', '='>
        > {
    };

    struct metric_name : identifier {
    };

    struct view_name : identifier {
    };

    // A name that's interpreted as either a view name or a metric name;
    // the VFL grammar defines <objectOfPredicate> as <constant>|<viewName>,
    // and <constant> is defined as <metricName>|<number>.
    // Therefore, <objectOfPredicate> is equivalent to <metricName>|<number>|<viewName>,
    // which doesn't make a lot of sense.
    struct object_name : identifier {
    };

    struct priority : sor<metric_name, number> {
    };

    struct constant : sor<metric_name, number> {
    };

    struct object_of_predicate : sor<object_name, number> {
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

    struct simple_connection : one<'-'> {
    };

    struct connection : sor<connection_with_predicate, simple_connection> {
    };

    struct view_connection : seq<
            opt<connection>,
            view
        > {
    };

    struct constraint : seq<
            ws,
            opt<orientation_prefix>,
            ws,
            opt<seq<leading_superview, opt<connection>>>,
            view,
            star<view_connection>,
            opt<seq<opt<connection>, trailing_superview>>,
            ws
        > {
    };

    struct grammar : seq<
            constraint,
            star<seq<ws, one<';'>, ws, constraint>>
        > {
    };

    // This template specialization lists the rules that will be stored in the parse tree.
    // They'll all still be recognized, but we don't need to look closely at all of them
    // (like the whitespace rules or the parentheses)
    template<typename Rule>
    using selector = parse_tree::selector<Rule,
        parse_tree::store_content::on<
            constraint,
            view,
            view_name,
            connection,
            orientation,
            predicate,
            object_name,
            relation,
            number,
            priority,
            constant,
            metric_name,
            simple_predicate,
            leading_superview,
            trailing_superview
        >
    >;
}

using namespace MelonDsDs::Vfl;


namespace pegtl = TAO_PEGTL_NAMESPACE;

using pegtl::parse_tree::node;

class invalid_node final : public std::invalid_argument {
public:
    invalid_node(string_view type, string_view content)
        : std::invalid_argument("Invalid node of type '" + std::string(type) + "' with content '" + std::string(content) + "'") {
    }

    explicit invalid_node(const node& node) : invalid_node(node.type, node.has_content() ? node.string_view() : "<no content>") {
    }
};

namespace {
    // By this point we know that the parse tree is valid,
    // but we don't yet know if the constraints or views are valid.
    // We'll validate those later.
    using namespace MelonDsDs::Vfl;

    [[nodiscard]] std::string NormalizeName(string_view view) noexcept {
        std::string name(view);

        std::transform(view.begin(), view.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
        name.erase(std::remove_if(name.begin(), name.end(), [](char c) { return c == '_'; } ), name.end());

        return name;
    }

    [[nodiscard]] Relation ParseRelation(string_view view) {
        ZoneScopedN(TracyFunction);
        if (view == "==") {
            return Relation::Equal;
        }

        if (view == "<=") {
            return Relation::LessEqual;
        }

        if (view == ">=") {
            return Relation::GreaterEqual;
        }

        throw std::invalid_argument("Invalid relation");
    }

    float ParseNumber(string_view view) {
        // Convert the number to a float
        float num = 0.0f;
        std::from_chars_result result = std::from_chars(view.begin(), view.end(), num);
        if (result.ec != std::errc()) {
            throw std::invalid_argument("Invalid number: " + std::string(view));
        }

        return num;
    }

    Priority ParsePriority(const node& node) {
        ZoneScopedN(TracyFunction);
        retro_assert(node.is_type<priority>());

        if (node.has_content()) {
            // If the priority has content, it can be a number or a metric name
            if (node.is_type<number>()) {
                return ParseNumber(node.string_view());
            } else if (node.is_type<metric_name>()) {
                return node.string();
            }
            else {
                throw invalid_node(node);
            }
        }

        // If no content, return an empty priority
        return std::monostate{};
    }

    [[nodiscard]] Predicate ParsePredicate(const node& node) {
        ZoneScopedN(TracyFunction);
        retro_assert(node.is_type<predicate>() || node.is_type<simple_predicate>());

        Predicate predicate;

        for (const auto& child: node.children) {
            if (child->is_type<relation>()) {
                // Set the relation
                predicate.relation = ParseRelation(child->string_view());
            } else if (child->is_type<object_name>()) {
                // Later we'll resolve this to a metric name or a view name
                predicate.object = NormalizeName(child->string());
            } else if (child->is_type<number>()) {
                predicate.object = ParseNumber(child->string_view());
            } else if (child->is_type<priority>()) {
                predicate.priority = ParsePriority(*child);
            } else {
                throw invalid_node(*child);
            }
        }

        return predicate;
    }

    [[nodiscard]] View ParseView(const node& node) {
        ZoneScopedN(TracyFunction);
        retro_assert(node.is_type<view>());

        View view;

        for (const auto& child: node.children) {
            if (child->is_type<view_name>()) {
                // Normalize the name by lowercasing it and removing
                view.name = NormalizeName(child->string_view());
            } else if (child->is_type<predicate>()) {
                view.predicates.emplace_back(ParsePredicate(*child));
            }
            else {
                throw invalid_node(*child);
            }
        }

        return view;
    }

    [[nodiscard]] Connection ParseConnection(const node& node) {
        ZoneScopedN(TracyFunction);
        retro_assert(node.is_type<connection>());

        Connection connection;

        for (const auto& child: node.children) {
            if (child->is_type<simple_connection>()) {
                // Simple connection, no predicates
                // This is just a '-' character
            } else if (child->is_type<predicate>()) {
                // Connection with predicates
                for (const auto& predicate_node: child->children) {
                    connection.predicates.emplace_back(ParsePredicate(*predicate_node));
                }
            }
            else if (child->is_type<simple_predicate>()) {

            }
            else {
                throw invalid_node(*child);
            }
        }

        return connection;
    }

    [[nodiscard]] Constraint ParseConstraint(const node& node) {
        ZoneScopedN(TracyFunction);

        retro_assert(node.is_type<constraint>());

        Constraint constraint;

        for (const auto& child: node.children) {
            if (child->is_type<orientation>()) {
                constraint.orientation = child->string_view() == "H" ? Orientation::Horizontal : Orientation::Vertical;
            } else if (child->is_type<view>()) {
                constraint.elements.emplace_back(ParseView(*child));
            } else if (child->is_type<leading_superview>()) {
                constraint.relative_to_superview_start = true;
            } else if (child->is_type<trailing_superview>()) {
                constraint.relative_to_superview_end = true;
            } else if (child->is_type<connection>()) {
                constraint.elements.emplace_back(ParseConnection(*child));
            }
            else {
                throw invalid_node(*child);
            }
        }

        return constraint;
    }
}

std::optional<Layout> MelonDsDs::Vfl::Parse(std::string_view vfl, string_view source) try {
    ZoneScopedN(TracyFunction);
    retro::debug("Parsing VFL: '{}'", vfl);
    memory_input input(vfl.data(), vfl.length(), source);
    auto root = parse_tree::parse<grammar, selector>(input);

    if (!root) {
        // Parsing failed, return empty optional
        return std::nullopt;
    }

    Layout layout;

    std::function<void(const std::unique_ptr<node>&, int)> print_node_recursively = [&print_node_recursively
        ](const std::unique_ptr<node>& n, int depth = 0) {
        if (!n) return;
        retro::debug("{:>{}}{}: '{}'", " ", depth * 2, n->type, n->has_content() ? n->string_view() : "<empty>");
        for (const auto& child: n->children) {
            print_node_recursively(child, depth + 1);
        }
    };

    print_node_recursively(root, 0);

    // root node has no type, so we don't need to check for it
    for (const auto& child: root->children) {
        // TODO: Parse the screen orientation
        if (child->is_type<constraint>()) {
            layout.constraints.emplace_back(ParseConstraint(*child));
        }
        else {
            throw invalid_node(*child);
        }
    }

    return layout;
} catch (const parse_error& e) {
    const auto& position = e.position_object();
    retro::error("{} at {}:{} from {}", e.message(), position.line, position.column, position.source);
    return std::nullopt;
} catch (const invalid_node& e) {
    retro::error("Invalid node: {}", e.what());
    return std::nullopt;
}

size_t MelonDsDs::Vfl::AnalyzeGrammarIssues() noexcept {
    // TODO: Temporarily override std::cerr so the logs appear in libretro
    return tao::pegtl::analyze<grammar>();
}
