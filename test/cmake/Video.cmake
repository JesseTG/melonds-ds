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

add_python_test(
    NAME "Core runs for multiple frames with OpenGL"
    TEST_MODULE opengl.core_loads_unloads
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_render_mode=opengl"
    REQUIRES_OPENGL
)

add_python_test(
    NAME "Core runs for multiple frames with OpenGL and software rendering"
    TEST_MODULE opengl.core_loads_unloads
    CONTENT "${NDS_ROM}"
    REQUIRES_OPENGL
)

add_python_test(
    NAME "Core falls back to software renderer if OpenGL is unavailable"
    TEST_MODULE opengl.core_falls_back_to_software
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_render_mode=opengl"
    REQUIRES_OPENGL
)

add_python_test(
    NAME "Core can switch from software to OpenGL mode at runtime"
    TEST_MODULE opengl.core_switch_software_to_opengl_runtime
    CONTENT "${NDS_ROM}"
    REQUIRES_OPENGL
)
