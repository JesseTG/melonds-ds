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


#ifndef MELONDS_DS_THREADS_HPP
#define MELONDS_DS_THREADS_HPP

#include <stdexcept>
#include <rthreads/rthreads.h>

namespace retro {
    class slock {
    public:
        slock();
        slock(const slock&) = delete;
        slock(slock&&) = delete;
        slock& operator=(const slock&) = delete;
        slock& operator=(slock&&) = delete;
        ~slock() noexcept;
        void lock() noexcept;
        void unlock() noexcept;
        bool try_lock() noexcept;
    private:
        slock_t* mutex;
    };
}

#endif //MELONDS_DS_THREADS_HPP
