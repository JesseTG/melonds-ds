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
#include <stdexcept>
#include <string>

namespace melonds
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

    /// Thrown when attempting to load a file that is not a valid NDS or GBA ROM.
    class invalid_rom_exception : public emulator_exception {
    public:
        explicit invalid_rom_exception(const std::string &what_arg) : emulator_exception(what_arg) {}
        invalid_rom_exception(const std::string &what_arg, const std::string &user_message) : emulator_exception(what_arg, user_message) {}
    };

    /// Thrown when attempting to use FreeBIOS with unsupported functionality.
    class unsupported_bios_exception : public emulator_exception {
    public:
        explicit unsupported_bios_exception(const std::string &what_arg) : emulator_exception(what_arg) {}
        unsupported_bios_exception(const std::string &what_arg, const std::string &user_message) : emulator_exception(what_arg, user_message) {}
    };
}

#endif //MELONDS_DS_EXCEPTIONS_HPP
