/*
    Copyright 2024 Jesse Talavera

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

#include "test.hpp"

#include <string/stdstring.h>

#include "core.hpp"
#include "environment.hpp"

namespace MelonDsDs
{
    // sshhh...don't tell anyone
    extern CoreState& Core;
}

extern "C" int libretropy_add_integers(int a, int b) {
    return a + b;
}

extern "C" bool libretropy_send_message(const char* message) {
    return retro::set_error_message("{}", message);
}

extern "C" const char* libretropy_get_option(const char* key) {
    retro_variable var { key, nullptr };
    bool ok = retro::get_variable(&var);

    return ok ? var.value : nullptr;
}

extern "C" unsigned libretropy_get_options_version() {
    unsigned version = 0;
    retro::environment(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version);

    return version;
}

extern "C" unsigned libretropy_get_message_version() {
    unsigned version = 0;
    retro::environment(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION, &version);

    return version;
}

extern "C" bool libretropy_get_input_bitmasks() {
    bool ok = false;

    return retro::environment(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, &ok);
}

extern "C" uint64_t libretropy_get_input_device_capabilities() {
    uint64_t caps = 0;
    retro::environment(RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES, &caps);

    return caps;
}

extern "C" const char* libretropy_get_system_directory() {
    const char* path = nullptr;
    bool ok = retro::environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &path);
    return ok ? path : nullptr;
}

extern "C" const char* libretropy_get_save_directory() {
    const char* path = nullptr;
    bool ok = retro::environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &path);
    return ok ? path : nullptr;
}

extern "C" bool libretropy_get_power(retro_device_power* power) {
    return retro::environment(RETRO_ENVIRONMENT_GET_DEVICE_POWER, power);
}

extern "C" bool melondsds_console_exists() {
    using namespace MelonDsDs;
    const melonDS::NDS* console = Core.GetConsole();

    return console != nullptr;
}

extern "C" bool melondsds_arm7_bios_native() {
    using namespace MelonDsDs;
    const melonDS::NDS* console = Core.GetConsole();

    return console ? console->IsLoadedARM7BIOSKnownNative() : false;
}

extern "C" bool melondsds_arm9_bios_native() {
    using namespace MelonDsDs;
    const melonDS::NDS* console = Core.GetConsole();

    return console ? console->IsLoadedARM7BIOSKnownNative() : false;
}

extern "C" bool melondsds_firmware_native() {
    using namespace MelonDsDs;
    const melonDS::NDS* console = Core.GetConsole();

    return console ? console->GetFirmware().GetHeader().Identifier != melonDS::GENERATED_FIRMWARE_IDENTIFIER : false;
}

extern "C" size_t melondsds_gba_rom_length() {
    using namespace MelonDsDs;
    const melonDS::NDS* console = Core.GetConsole();

    if (!(console && console->GetGBACart()))
        return 0;

    return console->GetGBACart()->GetROMLength();
}

extern "C" const uint8_t* melondsds_gba_rom() {
    using namespace MelonDsDs;
    const melonDS::NDS* console = Core.GetConsole();

    if (!(console && console->GetGBACart()))
        return nullptr;

    return console->GetGBACart()->GetROM();
}

extern "C" size_t melondsds_gba_sram_length() {
    using namespace MelonDsDs;
    const melonDS::NDS* console = Core.GetConsole();

    if (!(console && console->GetGBACart()))
        return 0;

    return console->GetGBACart()->GetSaveMemoryLength();
}

extern "C" const uint8_t* melondsds_gba_sram() {
    using namespace MelonDsDs;
    const melonDS::NDS* console = Core.GetConsole();

    if (!(console && console->GetGBACart()))
        return nullptr;

    return console->GetGBACart()->GetSaveMemory();
}

extern "C" int melondsds_analog_cursor_x() {
    using namespace MelonDsDs;
    return Core.GetInputState().JoystickTouchPosition().x;
}

extern "C" int melondsds_analog_cursor_y() {
    using namespace MelonDsDs;
    return Core.GetInputState().JoystickTouchPosition().y;
}

extern "C" int melondsds_screen_layout() {
    using namespace MelonDsDs;
    return static_cast<int>(Core.GetScreenLayoutData().Layout());
}


extern "C" retro_proc_address_t MelonDsDs::GetRetroProcAddress(const char* sym) noexcept {
    if (string_is_equal(sym, "libretropy_add_integers"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_add_integers);

    if (string_is_equal(sym, "libretropy_get_system_directory"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_get_system_directory);

    if (string_is_equal(sym, "libretropy_get_save_directory"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_get_save_directory);

    if (string_is_equal(sym, "libretropy_send_message"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_send_message);

    if (string_is_equal(sym, "libretropy_get_option"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_get_option);

    if (string_is_equal(sym, "libretropy_get_options_version"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_get_options_version);

    if (string_is_equal(sym, "libretropy_get_message_version"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_get_message_version);

    if (string_is_equal(sym, "libretropy_get_input_bitmasks"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_get_input_bitmasks);

    if (string_is_equal(sym, "libretropy_get_input_device_capabilities"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_get_input_device_capabilities);

    if (string_is_equal(sym, "libretropy_get_power"))
        return reinterpret_cast<retro_proc_address_t>(libretropy_get_power);

    if (string_is_equal(sym, "melondsds_console_exists"))
        return reinterpret_cast<retro_proc_address_t>(melondsds_console_exists);

    if (string_is_equal(sym, "melondsds_arm7_bios_native"))
        return reinterpret_cast<retro_proc_address_t>(melondsds_arm7_bios_native);

    if (string_is_equal(sym, "melondsds_arm9_bios_native"))
        return reinterpret_cast<retro_proc_address_t>(melondsds_arm9_bios_native);

    if (string_is_equal(sym, "melondsds_firmware_native"))
        return reinterpret_cast<retro_proc_address_t>(melondsds_firmware_native);

    if (string_is_equal(sym, "melondsds_gba_rom_length"))
        return reinterpret_cast<retro_proc_address_t>(melondsds_gba_rom_length);

    if (string_is_equal(sym, "melondsds_gba_rom"))
        return reinterpret_cast<retro_proc_address_t>(melondsds_gba_rom);

    if (string_is_equal(sym, "melondsds_gba_sram_length"))
        return reinterpret_cast<retro_proc_address_t>(melondsds_gba_sram_length);

    if (string_is_equal(sym, "melondsds_gba_sram"))
        return reinterpret_cast<retro_proc_address_t>(melondsds_gba_sram);

    if (string_is_equal(sym, "melondsds_analog_cursor_x"))
        return reinterpret_cast<retro_proc_address_t>(melondsds_analog_cursor_x);

    if (string_is_equal(sym, "melondsds_analog_cursor_y"))
        return reinterpret_cast<retro_proc_address_t>(melondsds_analog_cursor_y);

    if (string_is_equal(sym, "melondsds_screen_layout"))
        return reinterpret_cast<retro_proc_address_t>(melondsds_screen_layout);

    return nullptr;
}

