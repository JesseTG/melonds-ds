add_python_test(
    NAME "melonDS DS loads"
    TEST_SCRIPT "basics/core_loads.py"
)

add_python_test(
    NAME "Core system info is valid"
    TEST_SCRIPT "basics/core_system_info_valid.py"
)

add_python_test(
    NAME "Core sets frontend callbacks"
    TEST_SCRIPT "basics/core_sets_callbacks.py"
)

add_python_test(
    NAME "Core implements get_proc_address"
    TEST_SCRIPT "basics/core_get_proc_address.py"
)

add_python_test(
    NAME "Core runs init and deinit"
    TEST_SCRIPT "basics/core_run_init_deinit.py"
)

add_python_test(
    NAME "Core loads and unloads without content"
    TEST_SCRIPT "basics/core_loads_unloads_without_content.py"
)

add_python_test(
    NAME "Core loads and unloads with content"
    TEST_SCRIPT "basics/core_loads_unloads_with_content.py"
    CONTENT "${NDS_ROM}"
)

#
#add_python_test(
#    NAME "Core loads and unloads with subsystem content"
#    TEST_SCRIPT ""
#)
#

add_python_test(
    NAME "Core runs for one frame"
    TEST_SCRIPT "basics/core_run_frame.py"
)

add_python_test(
    NAME "Core runs for multiple frames"
    TEST_SCRIPT "basics/core_run_frames.py"
)

add_python_test(
    NAME "Core generates audio"
    TEST_SCRIPT "basics/core_generates_audio.py"
)

add_python_test(
    NAME "Core generates video"
    TEST_SCRIPT "basics/core_generates_video.py"
)

#add_python_test(
#    NAME "Core accepts button input"
#    TEST_SCRIPT "basics/core_accepts_button_input.py"
#)

#
#add_python_test(
#    NAME "Core accepts analog input"
#    TEST_SCRIPT ""
#)
#
#add_python_test(
#    NAME "Core accepts pointer input"
#    TEST_SCRIPT ""
#)

add_python_test(
    NAME "Core exposes emulated RAM"
    TEST_SCRIPT "basics/core_exposes_ram.py"
)

add_python_test(
    NAME "Core exposes emulated SRAM"
    TEST_SCRIPT "basics/core_exposes_sram.py"
)

add_python_test(
    NAME "Core logs output"
    TEST_SCRIPT "basics/core_logs_output.py"
)

#add_python_test(
#    NAME "Core accepts microphone input"
#    TEST_SCRIPT ""
#)