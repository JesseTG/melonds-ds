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

#include "exceptions.hpp"
#undef isnan
#include "format.hpp"

#include <optional>
#include <sstream>
#include <fmt/core.h>

#include "strings/en_us.hpp"

using std::optional;
using std::string;
using std::string_view;
using namespace MelonDsDs::strings::en_us;
using fmt::arg;

MelonDsDs::nds_firmware_not_bootable_exception::nds_firmware_not_bootable_exception(string_view path) noexcept
    : bios_exception(
    fmt::format(NativeFirmwareNotBootableProblem, arg("path", path)),
    FirmwareNotBootableSolution
) {
}

MelonDsDs::nds_firmware_not_bootable_exception::nds_firmware_not_bootable_exception() noexcept
    : bios_exception(
    BuiltInFirmwareNotBootableProblem,
    FirmwareNotBootableSolution
) {
}

MelonDsDs::wrong_firmware_type_exception::wrong_firmware_type_exception(
    std::string_view path,
    MelonDsDs::ConsoleType console,
    melonDS::Firmware::FirmwareConsoleType firmwareConsole
) noexcept : bios_exception(
    fmt::format(
        WrongFirmwareProblem,
        arg("path", path),
        arg("firmwareConsole", firmwareConsole),
        arg("console", console)
    ),
    fmt::format(
        WrongFirmwareSolution,
        arg("console", console)
    )
) {
}


MelonDsDs::dsi_region_mismatch_exception::dsi_region_mismatch_exception(
    string_view path,
    melonDS::DSi_NAND::ConsoleRegion region,
    melonDS::RegionMask regions
) noexcept
    : config_exception(
    fmt::format(
        WrongNandRegionProblem,
        arg("path", path),
        arg("region", region),
        arg("regions", regions)
    ),
    WrongNandRegionSolution
) {
}

MelonDsDs::dsi_no_firmware_found_exception::dsi_no_firmware_found_exception() noexcept
    : bios_exception(
    NoDsiFirmwareProblem,
    NoDsiFirmwareSolution
) {
}

MelonDsDs::firmware_missing_exception::firmware_missing_exception(std::string_view path) noexcept
    : bios_exception(
    fmt::format(NoFirmwareProblem, arg("path", path)),
    fmt::format(NoFirmwareSolution, arg("path", path))
) {
}

MelonDsDs::nds_sysfiles_incomplete_exception::nds_sysfiles_incomplete_exception() noexcept
    : bios_exception(
        IncompleteNdsSysfilesProblem,
        IncompleteNdsSysfilesSolution
) {
}

MelonDsDs::dsi_missing_bios_exception::dsi_missing_bios_exception(MelonDsDs::BiosType bios, string_view path) noexcept
    : bios_exception(
    fmt::format(MissingDsiBiosProblem, arg("bios", bios)),
    fmt::format(
        MissingDsiBiosSolution,
        arg("bios", bios),
        arg("path", path)
    )
) {
}

MelonDsDs::dsi_no_nand_found_exception::dsi_no_nand_found_exception() noexcept
    : bios_exception(
    NoDsiNandProblem,
    NoDsiNandSolution
) {
}

MelonDsDs::dsi_nand_missing_exception::dsi_nand_missing_exception(string_view path) noexcept
    : bios_exception(
    fmt::format(MissingDsiNandProblem, arg("path", path)),
    fmt::format(MissingDsiNandSolution, arg("path", path))
) {
}

MelonDsDs::dsi_nand_corrupted_exception::dsi_nand_corrupted_exception(string_view path) noexcept
    : bios_exception(
    fmt::format(CorruptDsiNandProblem, arg("path", path)),
    CorruptDsiNandSolution
) {
}

static std::string construct_missing_bios_message(const std::vector<std::string>& bios_files) {
    std::stringstream error;

    error << "Missing these BIOS files: ";
    for (size_t i = 0; i < bios_files.size(); ++i) {
        const std::string& file = bios_files[i];
        error << file;
        if (i < bios_files.size() - 1) {
            // If this isn't the last missing BIOS file...
            error << ", ";
        }
    }

    return error.str();
}

MelonDsDs::missing_bios_exception::missing_bios_exception(const std::vector<std::string>& bios_files)
    : bios_exception(construct_missing_bios_message(bios_files)) {
}