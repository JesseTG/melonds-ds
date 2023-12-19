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

#ifndef MELONDS_DS_CHRONO_HPP
#define MELONDS_DS_CHRONO_HPP

#include <chrono>

#if __cpp_lib_chrono < 201907L
#include <date/date.h>

namespace std::chrono {
    using date::hh_mm_ss;
    using date::month_day;
    using date::month;
    using date::day;
}
#endif

#endif //MELONDS_DS_CHRONO_HPP
