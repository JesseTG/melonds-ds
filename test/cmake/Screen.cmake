add_python_test(
    NAME "Core supports hybrid sceen ratio of 3:1"
    TEST_MODULE basics.core_run_frame
    CONTENT "${NDS_ROM}"
    CORE_OPTION melonds_hybrid_ratio=3
    CORE_OPTION melonds_screen_layout1=hybrid-top
)

add_python_test(
    NAME "Layout grammar avoids infinite loops"
    TEST_MODULE layout.grammar_avoids_infinite_loops
)


add_python_test(
    NAME "Layout parser accepts correct input"
    TEST_MODULE layout.parser_accepts_correct_input
)

add_python_test(
    NAME "Layout parser rejects incorrect input"
    TEST_MODULE layout.parser_rejects_incorrect_input
)