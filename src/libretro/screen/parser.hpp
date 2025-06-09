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

#include "environment.hpp"

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

   // Either a numeric value, view name, or metric name
   using PredicateObject = std::variant<float, std::string>;

   // Absent, a numeric value, or a metric name
   using Priority = std::variant<std::monostate, float, std::string>;

   // A predicate for view size or connection distance
   struct Predicate {
      Relation relation = Relation::Equal;
      PredicateObject object; // Constant, view name, or metric name
      Priority priority;
   };

   // Connection between views or to superview
   struct Connection {
      // If this is empty, then this is a simple connection
      std::vector<Predicate> predicates;
   };

   // A view in the format string
   struct View {
      std::string name;
      std::vector<Predicate> predicates;
   };

   using ConstraintElement = std::variant<View, Connection>;

   // The complete visual format string
   struct Constraint {
      std::vector<ConstraintElement> elements;
      Orientation orientation = Orientation::Horizontal;

      // "superview" in this case meaning the whole libretro screen
      bool relative_to_superview_start = false;
      bool relative_to_superview_end = false;

      // Helper to determine if we're working with width or height based on orientation
      std::string dimension_attr() const {
         return orientation == Orientation::Horizontal ? "width" : "height";
      }

      std::string position_attr() const {
         return orientation == Orientation::Horizontal ? "x" : "y";
      }
   };

   struct Layout {
      retro::ScreenOrientation rotation = retro::ScreenOrientation::Normal;
      std::vector<Constraint> constraints;
   };

   std::optional<Layout> Parse(std::string_view vfl, std::string_view source="visual_format_string");

   size_t AnalyzeGrammarIssues() noexcept;
}
