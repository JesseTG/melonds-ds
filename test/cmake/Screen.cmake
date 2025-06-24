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
