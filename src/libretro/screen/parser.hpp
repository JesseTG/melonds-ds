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

#pragma once

#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace MelonDsDs::Vfl {
   enum class Orientation {
      Horizontal,  // H: (default)
      Vertical     // V:
   };

   enum class Relation {
      Equal,        // ==
      LessEqual,    // <=
      GreaterEqual  // >=
   };

   // Either a metric name or a numeric value
   using Constant = std::variant<std::string, float>;
   using Priority = std::variant<std::string, float>;

   // A predicate for view size or connection distance
   struct Predicate {
      std::optional<Relation> relation = std::nullopt;
      std::variant<Constant, std::string> object; // Constant or view name
      std::optional<Priority> priority = std::nullopt;
   };

   // A list of predicates or a simple predicate
   struct PredicateList {
      std::variant<
         std::string,            // Metric name
         double,                 // Number
         std::vector<Predicate> // List of predicates
      > value;

      // Resolves a simple predicate to its value
      double resolve_simple(const std::map<std::string, double>& metrics) const {
         if (std::holds_alternative<double>(value)) {
            return std::get<double>(value);
         } else if (std::holds_alternative<std::string>(value)) {
            const auto& name = std::get<std::string>(value);
            auto it = metrics.find(name);
            if (it == metrics.end()) {
               throw std::runtime_error("Unknown metric: " + name);
            }
            return it->second;
         }
         throw std::runtime_error("Not a simple predicate");
      }

      bool is_simple() const {
         return std::holds_alternative<double>(value) ||
                std::holds_alternative<std::string>(value);
      }
   };

   // Connection between views or to superview
   struct Connection {
      std::optional<PredicateList> predicates;

      // Default standard spacing if no predicates
      /*double get_spacing(const std::map<std::string, double>& metrics, double standard_space = 8.0) const {
         if (!predicates) {
            return standard_space;
         }

         if (predicates->is_simple()) {
            return predicates->resolve_simple(metrics);
         }

         // For complex predicate lists, we just use the first one
         // (This is a simplification - a more complete implementation would handle all predicates)
         const auto& predicate_list = std::get<std::vector<Predicate>>(predicates->value);
         if (!predicate_list.empty()) {
            const auto& predicate = predicate_list[0];
            if (std::holds_alternative<Constant>(predicate.object)) {
               return std::get<Constant>(predicate.object).resolve(metrics);
            }
         }

         return standard_space;
      }*/
   };

   // A view in the format string
   struct View {
      std::string name;
      std::optional<std::vector<Predicate>> predicates;
   };

   // The complete visual format string
   struct VflConstraint {
      Orientation orientation = Orientation::Horizontal;
      bool has_leading_superview = false;
      bool has_trailing_superview = false;
      std::vector<View> views;
      std::vector<Connection> connections;

      // Helper to determine if we're working with width or height based on orientation
      std::string dimension_attr() const {
         return orientation == Orientation::Horizontal ? "width" : "height";
      }

      std::string position_attr() const {
         return orientation == Orientation::Horizontal ? "x" : "y";
      }
   };

   std::optional<std::vector<VflConstraint>> Parse(std::string_view vfl, std::string_view source="visual_format_string");

   size_t AnalyzeGrammarIssues() noexcept;
}
