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
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core region is correct"
    TEST_MODULE basics.core_region_correct
)

add_python_test(
    NAME "Core runs init and deinit"
    TEST_MODULE basics.core_run_init_deinit
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core loads and unloads with content"
    TEST_MODULE basics.core_loads_unloads_with_content
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core runs for one frame"
    TEST_MODULE basics.core_run_frame
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core runs for multiple frames"
    TEST_MODULE basics.core_run_frames
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core resets emulator state"
    TEST_MODULE basics.core_resets
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core generates audio"
    TEST_MODULE basics.core_generates_audio
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core generates video"
    TEST_MODULE basics.core_generates_video
    CONTENT "${NDS_ROM}"
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
    TEST_MODULE basics.core_saves_state
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core saves and loads state"
    TEST_MODULE basics.core_saves_and_loads_state
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core exposes emulated RAM"
    TEST_MODULE basics.core_exposes_ram
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core exposes emulated SRAM"
    TEST_MODULE basics.core_exposes_sram
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core applies cheats"
    TEST_MODULE basics.core_applies_cheats
    CONTENT "${NDS_ROM}"
)

# Environment Calls
# (these also serve as good tests for libretro.py itself)

add_python_test(
    NAME "Core rotates the screen"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core can send messages (API V0)"
    TEST_MODULE basics.core_sends_messages_v0
    NDS_SYSFILES
)

add_python_test(
    NAME "Core can shut down internally"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core gets the frontend's system directory"
    TEST_MODULE basics.core_gets_system_directory
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core sets pixel format to XRGB8888"
    TEST_MODULE basics.core_sets_pixel_format
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core sets input descriptors"
    TEST_MODULE basics.core_defines_input_descriptors
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core can fetch option values"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sets options (V0 API)"
    TEST_MODULE basics.core_defines_options_v0
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core tests for option updates"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core registers support for no-content mode"
    TEST_MODULE basics.core_registers_no_content_support
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core loads and unloads without content"
    TEST_MODULE basics.core_loads_unloads_without_content
)

add_python_test(
    NAME "Core logs output"
    TEST_MODULE basics.core_logs_output
    NDS_SYSFILES
)

add_python_test(
    NAME "Core gets the frontend's save directory"
    TEST_MODULE basics.core_gets_save_directory
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core sets retro_get_proc_address_interface"
    TEST_MODULE basics.core_get_proc_address
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core defines subsystems"
    TEST_MODULE basics.core_defines_subsystems
    NDS_SYSFILES
)

add_python_test(
    NAME "Core loads and unloads with subsystem content"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core defines controller info"
    TEST_MODULE basics.core_defines_controller_info
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core sets geometry at runtime"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core registers support for achievements"
    TEST_MODULE basics.core_registers_achievement_support
    CONTENT "${NDS_ROM}"
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
    TEST_MODULE basics.core_gets_options_version
    NDS_SYSFILES
)

add_python_test(
    NAME "Core sets options (V1 API)"
    TEST_MODULE basics.core_defines_options_v1
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core hides options when the frontend updates them"
    TEST_MODULE basics.core_sets_options_visibility
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core gets message API version"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core sends messages (API V1)"
    TEST_MODULE basics.core_sends_messages_v1
    NDS_SYSFILES
)

add_python_test(
    NAME "Core sets content info overrides"
    TEST_MODULE basics.core_defines_content_info_overrides
    NDS_SYSFILES
)

add_python_test(
    NAME "Core sets options (V2 API)"
    TEST_MODULE basics.core_defines_options_v2
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core sets options visibility update callback"
    TEST_MODULE basics.core_sets_options_visibility_update_callback
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Core accepts microphone input"
    TEST_MODULE ""
)

add_python_test(
    NAME "Core queries device power state"
    TEST_MODULE ""
)




