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

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <rhea/variable.hpp>
#include <rhea/constraint.hpp>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include "environment.hpp"

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
            pegtl::string<'=', '='>,
            pegtl::string<'<', '='>,
            pegtl::string<'>', '='>
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

    struct visual_format_constraint : seq<
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

    struct visual_format_string : seq<
            visual_format_constraint,
            star<seq<one<';'>, visual_format_constraint>>
        > {
    };

    // Main grammar rule
    struct grammar : must<visual_format_string, eof> {
    };

    // This template specialization declares that nodes will be created
    // for the `grammar` rule
    template<typename Rule>
    using selector = parse_tree::selector<Rule,
        parse_tree::store_content::on<grammar>
    >;
}

using namespace MelonDsDs::Vfl;


namespace pegtl = TAO_PEGTL_NAMESPACE;

using pegtl::parse_tree::node;

// Convert parse tree to AST
/*
struct transform {
    static Relation to_relation(const std::string& rel) {
        if (rel == "==") return Relation::Equal;
        if (rel == "<=") return Relation::LessEqual;
        if (rel == ">=") return Relation::GreaterEqual;
        throw std::runtime_error("Invalid relation: " + rel);
    }

    static Predicate parse_predicate(std::unique_ptr<pegtl::parse_tree::node>& n) {
        Predicate predicate;

        for (auto& child: n->children) {
            if (child->is_type<relation>()) {
                predicate.relation = to_relation(child->string());
            } else if (child->is_type<metric_name>()) {
                // This could be a metric name in the object position
                predicate.object = Constant::from_metric(child->string());
            } else if (child->is_type<number>()) {
                // This could be a number in the object position
                predicate.object = Constant::from_number(std::stod(child->string()));
            } else if (child->is_type<view_name>()) {
                // This could be a view name in the object position
                predicate.object = child->string();
            } else if (child->is_type<priority_suffix>()) {
                // Process priority
                for (auto& prio_child: child->children) {
                    if (prio_child->is_type<metric_name>()) {
                        predicate.priority = Priority::from_metric(prio_child->string());
                    } else if (prio_child->is_type<number>()) {
                        predicate.priority = Priority::from_number(std::stod(prio_child->string()));
                    }
                }
            }
        }

        return predicate;
    }
};
*/

std::optional<std::vector<VflConstraint>> MelonDsDs::Vfl::Parse(std::string_view vfl, string_view source) {
    memory_input input(vfl.data(), vfl.length(), source);
    auto root = parse_tree::parse<grammar>(input);

    if (!root) {
        // Parsing failed, return empty optional
        return std::nullopt;
    }

    std::vector<VflConstraint> constraints;

    // Process the visual format string node
    for (auto& child: root->children) {
        retro::debug("{}", child->string_view());
        /*
        if (child->is_type<orientation>()) {
            vfs.orientation = (child->string() == "V") ? Orientation::Vertical : Orientation::Horizontal;
        } else if (child->is_type<superview>()) {
            // Determine if leading or trailing based on position
            if (vfs.views.empty()) {
                vfs.has_leading_superview = true;
            } else {
                vfs.has_trailing_superview = true;
            }
        } else if (child->is_type<view>()) {
            View view;

            // Extract view name
            for (auto& view_child: child->children) {
                if (view_child->is_type<view_name>()) {
                    view.name = view_child->string();
                } else if (view_child->is_type<predicate_list_with_parens>()) {
                    std::vector<Predicate> predicates;

                    for (auto& pred_child: view_child->children) {
                        if (pred_child->is_type<predicate>()) {
                            predicates.push_back(parse_predicate(pred_child));
                        }
                    }

                    if (!predicates.empty()) {
                        view.predicates = predicates;
                    }
                }
            }

            vfs.views.push_back(view);
        } else if (child->is_type<connection>() || child->is_type<connection_with_predicate>()) {
            Connection connection;

            // Extract predicates from connection
            for (auto& conn_child: child->children) {
                if (conn_child->is_type<predicate_list>() ||
                    conn_child->is_type<simple_predicate>() ||
                    conn_child->is_type<predicate_list_with_parens>()) {
                    if (conn_child->is_type<simple_predicate>()) {
                        // Simple predicate (number or metric)
                        PredicateList predList;

                        if (conn_child->children[0]->is_type<number>()) {
                            predList.value = std::stod(conn_child->children[0]->string());
                        } else if (conn_child->children[0]->is_type<metric_name>()) {
                            predList.value = conn_child->children[0]->string();
                        }

                        connection.predicates = predList;
                    } else if (conn_child->is_type<predicate_list_with_parens>()) {
                        // Complex predicate list
                        PredicateList predList;
                        std::vector<Predicate> predicates;

                        for (auto& pred_child: conn_child->children) {
                            if (pred_child->is_type<predicate>()) {
                                predicates.push_back(parse_predicate(pred_child));
                            }
                        }

                        predList.value = predicates;
                        connection.predicates = predList;
                    }
                }
            }

            vfs.connections.push_back(connection);
        }
        */
    }

    return constraints;
}

size_t MelonDsDs::Vfl::AnalyzeGrammarIssues() noexcept {
    // TODO: Temporarily override std::cerr so the logs appear in libretro
    return tao::pegtl::analyze<grammar>();
}
