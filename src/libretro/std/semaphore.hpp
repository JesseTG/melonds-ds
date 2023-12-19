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

#ifndef MELONDS_DS_STD_SEMAPHORE_HPP
#define MELONDS_DS_STD_SEMAPHORE_HPP

#ifdef __cpp_lib_semaphore
#include <semaphore>
#elif defined(WIN32)
#include <win_semaphore.hpp>

namespace std {
    template <std::ptrdiff_t least_max_value = 0x7FFFFFFF>
    using counting_semaphore = yamc::win::counting_semaphore<least_max_value>;
}
#elif defined(__APPLE__)
#include <gcd_semaphore.hpp>

namespace std {
    template <std::ptrdiff_t least_max_value = std::numeric_limits<long>::max()>
    using counting_semaphore = yamc::gcd::counting_semaphore<least_max_value>;
}
#elif defined(__unix__)
#include <posix_semaphore.hpp>

namespace std {
    template <std::ptrdiff_t least_max_value = SEM_VALUE_MAX>
    using counting_semaphore = yamc::posix::counting_semaphore<least_max_value>;
}
#else
#include <yamc_semaphore.hpp>

namespace std {
    template <std::ptrdiff_t least_max_value = YAMC_SEMAPHORE_LEAST_MAX_VALUE>
    using counting_semaphore = yamc::counting_semaphore<least_max_value>;
}
#endif

#endif // MELONDS_DS_STD_SEMAPHORE_HPP
