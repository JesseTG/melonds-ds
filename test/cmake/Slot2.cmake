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