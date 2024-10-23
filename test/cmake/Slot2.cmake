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
    TEST_MODULE ""
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=rumble-pak"
    DISABLED
) # TODO: trigger the rumble pak, check the rumble interface

add_python_test(
    NAME "Rumble Pak is removed after reset when disabling it"
    TEST_MODULE ""
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=rumble-pak"
    DISABLED
)

add_python_test(
    NAME "Rumble Pak is added after reset when enabling it"
    TEST_MODULE ""
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=rumble-pak"
    DISABLED
)

add_python_test(
    NAME "Memory Expansion Pak is removed after reset when disabling it"
    TEST_MODULE ""
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=rumble-pak"
    DISABLED
)

add_python_test(
    NAME "Memory Expansion Pak is added after reset when enabling it"
    TEST_MODULE ""
    CONTENT "${PERIPH_SLOT2_NDS}"
    CORE_OPTION "melonds_slot2_device=rumble-pak"
    DISABLED
)