
add_python_test(
    NAME "Cheats persist after retro_reset"
    TEST_MODULE cheats.persist_after_reset
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Resetting cheats clears them"
    TEST_MODULE cheats.cleared_on_cheat_reset
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Cheats can be updated individually"
    TEST_MODULE cheats.can_be_updated_individually
    CONTENT "${NDS_ROM}"
    DISABLED
)


add_python_test(
    NAME "Cheats can be disabled individually"
    TEST_MODULE cheats.can_be_disabled_individually
    CONTENT "${NDS_ROM}"
    DISABLED
)

add_python_test(
    NAME "Cheats can be toggled on the error screen"
    TEST_MODULE cheats.can_be_toggled_on_error_screen
    CONTENT "${NDS_ROM}"
    DISABLED
)

add_python_test(
    NAME "Invalid cheats are not enabled"
    TEST_MODULE cheats.not_enabled_if_invalid
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Cheats can be disabled if they're invalid"
    TEST_MODULE cheats.can_be_disabled_if_invalid
    CONTENT "${NDS_ROM}"
    DISABLED
)
