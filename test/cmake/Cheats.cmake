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
    NAME "Invalid cheats are not enabled"
    TEST_MODULE cheats.not_enabled_if_invalid
    CONTENT "${NDS_ROM}"
)
