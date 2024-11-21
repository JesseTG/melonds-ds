add_python_test(
    NAME "Core detects Memory Expansion Pak"
    TEST_MODULE "slot2.supports_expansion_pak"
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=expansion-pak"
)

add_python_test(
    NAME "Core detects Rumble Pak"
    TEST_MODULE "slot2.supports_rumble_pak"
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=rumble-pak"
)

add_python_test(
    NAME "Core triggers Rumble Pak"
    TEST_MODULE "slot2.triggers_rumble_pak"
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=rumble-pak"
)

add_python_test(
    NAME "Core does not trigger Rumble Pak if one isn't inserted"
    TEST_MODULE "slot2.doesnt_trigger_rumble_pak_if_not_inserted"
    CONTENT "${PERIPH_SLOT2_NDS}"
)

add_python_test(
    NAME "Rumble Pak is removed after reset when disabled"
    TEST_MODULE "slot2.removes_rumble_pak_after_reset"
    CONTENT "${PERIPH_SLOT2_NDS}"
)

add_python_test(
    NAME "Rumble Pak is inserted after reset when enabled"
    TEST_MODULE "slot2.inserts_rumble_pak_after_reset"
    CONTENT "${PERIPH_SLOT2_NDS}"
)

add_python_test(
    NAME "Memory Expansion Pak is removed after reset when disabling it"
    TEST_MODULE "slot2.removes_memory_pak_after_reset"
    CONTENT "${PERIPH_SLOT2_NDS}"
)

add_python_test(
    NAME "Memory Expansion Pak is added after reset when enabling it"
    TEST_MODULE "slot2.inserts_memory_pak_after_reset"
    CONTENT "${PERIPH_SLOT2_NDS}"
)

add_python_test(
    NAME "Memory Expansion Pak not inserted if GBA ROM is explicitly loaded"
    TEST_MODULE slot2.doesnt_insert_accessory_with_gba_rom
    SUBSYSTEM gbanosav
    CONTENT "${PERIPH_SLOT2_NDS}"
    CONTENT "${GBA_ROM}"
    CORE_OPTION "melonds_slot2_device=expansion-pak"
)

add_python_test(
    NAME "Rumble Pak not inserted if GBA ROM is explicitly loaded"
    TEST_MODULE slot2.doesnt_insert_accessory_with_gba_rom
    SUBSYSTEM gbanosav
    CONTENT "${PERIPH_SLOT2_NDS}"
    CONTENT "${GBA_ROM}"
    CORE_OPTION "melonds_slot2_device=rumble-pak"
)

add_python_test(
    NAME "Core detects Solar Sensor with fake Boktai ROM"
    TEST_MODULE slot2.supports_solar_sensor_from_stub_rom
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=solar1"
)

add_python_test(
    NAME "Solar Sensor is inserted after reset when enabled"
    TEST_MODULE slot2.inserts_solar_sensor_after_reset
    CONTENT "${PERIPH_SLOT2_NDS}"
)

add_python_test(
    NAME "Solar Sensor is removed after reset when disabled"
    TEST_MODULE slot2.removes_solar_sensor_after_reset
    CONTENT "${PERIPH_SLOT2_NDS}"
)

add_python_test(
    NAME "Core gets Solar Sensor input from buttons"
    TEST_MODULE slot2.solar_sensor_buttons
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=solar1"
)

add_python_test(
    NAME "Core gets Solar Sensor input from mouse wheel"
    TEST_MODULE slot2.solar_sensor_mouse_wheel
    CONTENT "${PERIPH_SLOT2_NDS}"
)

add_python_test(
    NAME "Core gets Solar Sensor input from sensor interface"
    TEST_MODULE slot2.solar_sensor_interface
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=solar1"
)

add_python_test(
    NAME "Core doesn't enable light sensor if disabled"
    TEST_MODULE slot2.disabling_host_sensor_doesnt_enable_it
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=solar1"
)

add_python_test(
    NAME "Core falls back to button input for the Solar Sensor if no light sensor is available"
    TEST_MODULE slot2.solar_sensor_falls_back_to_buttons
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=solar1"
)
