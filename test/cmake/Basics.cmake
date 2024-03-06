# Basic Functionality

add_python_test(
    NAME "Core loads"
    TEST_MODULE basics.core_loads
)

add_python_test(
    NAME "Core sets frontend callbacks"
    TEST_MODULE basics.core_sets_callbacks
)

add_python_test(
    NAME "Core API version is 1"
    TEST_MODULE basics.core_api_version_correct
)

add_python_test(
    NAME "Core system info is correct"
    TEST_MODULE basics.core_system_info_valid
)

add_python_test(
    NAME "Core system AV info is correct"
    TEST_MODULE basics.core_system_av_info_correct
)

add_python_test(
    NAME "Core region is correct"
    TEST_MODULE basics.core_region_correct
)

add_python_test(
    NAME "Core runs init and deinit"
    TEST_MODULE "basics.core_run_init_deinit"
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core loads and unloads with content"
    TEST_MODULE "basics.core_loads_unloads_with_content"
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core runs for one frame"
    TEST_MODULE "basics.core_run_frame"
)

add_python_test(
    NAME "Core runs for multiple frames"
    TEST_MODULE "basics.core_run_frames"
)

add_python_test(
    NAME "Core resets emulator state"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core generates audio"
    TEST_MODULE "basics.core_generates_audio"
)

add_python_test(
    NAME "Core generates video"
    TEST_MODULE "basics.core_generates_video"
)

add_python_test(
    NAME "Core accepts button input"
    TEST_MODULE "basics.core_accepts_button_input"
)

add_python_test(
    NAME "Core accepts analog input"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core accepts pointer input"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core saves state"
    TEST_MODULE "basics.core_saves_state"
)

add_python_test(
    NAME "Core saves and loads state"
    TEST_MODULE "basics.core_saves_and_loads_state"
)

add_python_test(
    NAME "Core exposes emulated RAM"
    TEST_MODULE "basics.core_exposes_ram"
)

add_python_test(
    NAME "Core exposes emulated SRAM"
    TEST_MODULE "basics.core_exposes_sram"
)

add_python_test(
    NAME "Core applies cheats"
    TEST_MODULE ""
)

# Environment Calls
# (these also serve as good tests for libretro.py itself)

add_python_test(
    NAME "Core rotates the screen"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core can send messages (API V0)"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core can shut down internally"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core gets the frontend's system directory"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sets pixel format to XRGB8888"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sets input descriptors"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core can fetch option values"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sets options (V0 API)"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core tests for option updates"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core registers support for no-content mode"
    TEST_MODULE "basics.core_registers_no_content_support"
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core loads and unloads without content"
    TEST_MODULE "basics.core_loads_unloads_without_content"
)

add_python_test(
    NAME "Core logs output"
    TEST_MODULE "basics.core_logs_output"
    ARM7_BIOS
    ARM9_BIOS
    NDS_FIRMWARE
)

add_python_test(
    NAME "Core gets the frontend's save directory"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sets retro_get_proc_address_interface"
    TEST_MODULE "basics.core_get_proc_address"
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core defines subsystems"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core loads and unloads with subsystem content"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core defines controller info"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sets geometry at runtime"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core registers support for achievements"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core gets VFS interface"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core gets input bitmask support"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core gets options API version"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sets options (V1 API)"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core can set options visibility"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core gets message API version"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sends messages (API V1)"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sets content info overrides"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sets options (V2 API)"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sets options visibility display update callback"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core accepts microphone input"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core queries device power state"
    TEST_MODULE ""
)




