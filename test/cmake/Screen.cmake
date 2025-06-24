add_python_test(
    NAME "Core supports hybrid sceen ratio of 3:1"
    TEST_MODULE basics.core_run_frame
    CONTENT "${NDS_ROM}"
    CORE_OPTION melonds_hybrid_ratio=3
    CORE_OPTION melonds_screen_layout1=hybrid-top
)

add_python_test(
    NAME "Built-in layout config parses"
    TEST_MODULE layout.builtin_parses
)

add_python_test(
    NAME "Built-in layout config saves to disk if absent"
    TEST_MODULE layout.builtin_saves_to_disk
    DISABLED
)

add_python_test(
    NAME "Built-in layout config does not save to disk if present"
    TEST_MODULE layout.builtin_does_not_save_to_disk
    DISABLED
)

add_python_test(
    NAME "Valid layout definitions parse successfully"
    TEST_MODULE layout.valid_parses
)

add_python_test(
    NAME "Invalid layout definitions fail to parse"
    TEST_MODULE layout.invalid_doesnt_parse
)
