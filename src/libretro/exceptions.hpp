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

#ifndef MELONDS_DS_EXCEPTIONS_HPP
#define MELONDS_DS_EXCEPTIONS_HPP

#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <DSi_NAND.h>
#include "config/config.hpp"

namespace MelonDsDs
{
    class emulator_exception : public std::runtime_error {
    public:
        explicit emulator_exception(const std::string &what_arg) : std::runtime_error(what_arg), _user_message(what_arg) {}
        emulator_exception(const std::string &what_arg, const std::string &user_message) : std::runtime_error(what_arg), _user_message(user_message) {}

        const char *user_message() const noexcept {
            return _user_message.c_str();
        }
    protected:
        emulator_exception() : std::runtime_error(""), _user_message("") {}
    private:
        std::string _user_message;
    };

    /// An environment call failed, and there's no way to recover.
    class environment_exception : public emulator_exception {
    public:
        using emulator_exception::emulator_exception;
    };

    /// Thrown when attempting to load a file that is not a valid NDS or GBA ROM.
    class invalid_rom_exception : public emulator_exception {
    public:
        using emulator_exception::emulator_exception;
    };

    /// Thrown when a game cannot be loaded with the current configuration.
    /// This should not stop the core;
    /// instead, it should lead to an error screen
    /// so that the user has a chance to make corrections.
    class config_exception : public emulator_exception {
    public:
        using emulator_exception::emulator_exception;
    };

    /// Thrown when there's a problem with the BIOS, firmware, or NAND configuration.
    class bios_exception : public config_exception {
    public:
        using config_exception::config_exception;
    };

    class dsi_region_mismatch_exception : public config_exception {
    public:
        dsi_region_mismatch_exception(std::string_view nandName, melonDS::DSi_NAND::ConsoleRegion nandRegion, melonDS::RegionMask gameRegionMask) noexcept;
    };

    class dsi_no_firmware_found_exception : public bios_exception {
    public:
        dsi_no_firmware_found_exception() noexcept;
    };

    class firmware_missing_exception : public bios_exception {
    public:
        firmware_missing_exception(std::string_view firmwareName) noexcept;
    };

    class wrong_firmware_type_exception: public bios_exception {
    public:
        wrong_firmware_type_exception(
            std::string_view  firmwareName,
            MelonDsDs::ConsoleType consoleType,
            melonDS::Firmware::FirmwareConsoleType firmwareConsoleType
        ) noexcept;
    };

    class nds_firmware_not_bootable_exception : public bios_exception {
    public:
        explicit nds_firmware_not_bootable_exception() noexcept;
        explicit nds_firmware_not_bootable_exception(std::string_view path) noexcept;
    };


    class nds_sysfiles_incomplete_exception : public bios_exception {
    public:
        explicit nds_sysfiles_incomplete_exception() noexcept;
    };

    /// Thrown when attempting to load a required BIOS file that is missing.
    class dsi_missing_bios_exception : public bios_exception {
    public:
        using bios_exception::bios_exception;
        explicit dsi_missing_bios_exception(BiosType bios, std::string_view biosName) noexcept;
    };

    class dsi_no_nand_found_exception : public bios_exception {
    public:
        dsi_no_nand_found_exception() noexcept;
    };

    class dsi_nand_missing_exception : public bios_exception {
    public:
        explicit dsi_nand_missing_exception(std::string_view biosName) noexcept;
    };

    class dsi_nand_corrupted_exception : public bios_exception {
    public:
        explicit dsi_nand_corrupted_exception(std::string_view biosName) noexcept;
    };

    /// Thrown when there's a problem with the BIOS, firmware, or NAND configuration.
    class unsupported_bios_exception : public bios_exception {
    public:
        using bios_exception::bios_exception;
    };

    /// Thrown when attempting to load a required BIOS file that is missing.
    class missing_bios_exception : public bios_exception {
    public:
        explicit missing_bios_exception(const std::vector<std::string>& bios_files);
        using bios_exception::bios_exception;
    };

    class missing_metadata_exception : public emulator_exception {
    public:
        using emulator_exception::emulator_exception;
    };

    class opengl_exception : public emulator_exception {
    public:
        using emulator_exception::emulator_exception;
    };

    class shader_compilation_failed_exception : public opengl_exception {
    public:
        using opengl_exception::opengl_exception;
    };

    class opengl_not_initialized_exception : public opengl_exception {
    public:
        using opengl_exception::opengl_exception;
    };
}

#endif //MELONDS_DS_EXCEPTIONS_HPP
