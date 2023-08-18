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

#ifndef MELONDS_DS_CPU_HPP
#define MELONDS_DS_CPU_HPP

#include <array>
#include <libretro.h>

#include "../constants.hpp"

namespace melonds::config::definitions {
    template<retro_language L>
    constexpr std::array CpuOptionDefinitions {
#ifdef JIT_ENABLED
        retro_core_option_v2_definition {
            config::cpu::JIT_ENABLE,
            "JIT Recompiler",
            nullptr,
            "Recompiles emulated machine code into native code as it runs, "
            "considerably improving performance over plain interpretation. "
            "Restart required to take effect. "
            "If unsure, leave enabled.",
            nullptr,
            melonds::config::cpu::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::ENABLED
        },
        retro_core_option_v2_definition {
            config::cpu::JIT_BLOCK_SIZE,
            "Block Size",
            nullptr,
            nullptr,
            nullptr,
            melonds::config::cpu::CATEGORY,
            {
                {"1", nullptr},
                {"2", nullptr},
                {"3", nullptr},
                {"4", nullptr},
                {"5", nullptr},
                {"6", nullptr},
                {"7", nullptr},
                {"8", nullptr},
                {"9", nullptr},
                {"10", nullptr},
                {"11", nullptr},
                {"12", nullptr},
                {"13", nullptr},
                {"14", nullptr},
                {"15", nullptr},
                {"16", nullptr},
                {"17", nullptr},
                {"18", nullptr},
                {"19", nullptr},
                {"20", nullptr},
                {"21", nullptr},
                {"22", nullptr},
                {"23", nullptr},
                {"24", nullptr},
                {"25", nullptr},
                {"26", nullptr},
                {"27", nullptr},
                {"28", nullptr},
                {"29", nullptr},
                {"30", nullptr},
                {"31", nullptr},
                {"32", nullptr},
                {nullptr, nullptr},
            },
            "32"
        },
        retro_core_option_v2_definition {
            config::cpu::JIT_BRANCH_OPTIMISATIONS,
            "Branch Optimizations",
            nullptr,
            nullptr,
            nullptr,
            melonds::config::cpu::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::ENABLED
        },
        retro_core_option_v2_definition {
            config::cpu::JIT_LITERAL_OPTIMISATIONS,
            "Literal Optimizations",
            nullptr,
            nullptr,
            nullptr,
            melonds::config::cpu::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
            melonds::config::values::ENABLED
        },
#ifdef HAVE_JIT_FASTMEM
        // Fastmem uses SIGSEGV for reasons I don't exactly understand,
        // but I do know that it makes using debuggers a pain
        // due to the constant breaks at each SIGSEGV.
        // So it's turned off by default in debug builds.
        retro_core_option_v2_definition {
            config::cpu::JIT_FAST_MEMORY,
            "Fast Memory",
            nullptr,
#ifndef NDEBUG
            "Disable this if running melonDS DS through a debugger, "
            "otherwise the constant (but expected) SIGSEGVs will get annoying. "
            #endif
            "Takes effect at next restart. "
            "If unsure, leave enabled.",
            nullptr,
            melonds::config::cpu::CATEGORY,
            {
                {melonds::config::values::DISABLED, nullptr},
                {melonds::config::values::ENABLED, nullptr},
                {nullptr, nullptr},
            },
#ifdef NDEBUG
            melonds::config::values::ENABLED
#else
            melonds::config::values::DISABLED
#endif
        },
#endif
#endif
    };
}
#endif //MELONDS_DS_CPU_HPP
