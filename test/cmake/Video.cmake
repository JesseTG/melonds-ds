# See https://github.com/JesseTG/melonds-ds/issues/70
add_python_test(
    NAME "Core unloads with threaded software rendering"
    TEST_MODULE basics.core_run_frames
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_sysfile_mode=builtin"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_threaded_renderer=enabled"
)