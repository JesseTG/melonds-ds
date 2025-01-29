add_python_test(
    NAME "Core falls back to FreeBIOS if ARM7 BIOS is missing (NDS)"
    CONTENT "${NDS_ROM}"
    TEST_MODULE errors.core_falls_back_to_freebios
    CORE_OPTION melonds_console_mode=ds
    CORE_OPTION melonds_sysfile_mode=native
    CORE_OPTION melonds_boot_mode=direct
    ARM9_BIOS
)

add_python_test(
    NAME "Core falls back to FreeBIOS if ARM9 BIOS is missing (NDS)"
    CONTENT "${NDS_ROM}"
    TEST_MODULE errors.core_falls_back_to_freebios
    CORE_OPTION melonds_console_mode=ds
    CORE_OPTION melonds_sysfile_mode=native
    CORE_OPTION melonds_boot_mode=direct
    ARM7_BIOS
)

add_python_test(
    NAME "Core falls back to built-in firmware if native firmware is missing (NDS)"
    CONTENT "${NDS_ROM}"
    TEST_MODULE errors.core_falls_back_to_freebios
    CORE_OPTION melonds_console_mode=ds
    CORE_OPTION melonds_sysfile_mode=native
    CORE_OPTION melonds_boot_mode=direct
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
    ARM7_BIOS
    ARM9_BIOS
)

add_python_test(
    NAME "Loading invalid ROM does not cause a crash"
    TEST_MODULE basics.core_loads_unloads_with_content
    CONTENT "$<TARGET_FILE:melondsds_libretro>"
    WILL_FAIL
)

add_python_test(
    NAME "Loading save data as a ROM does not cause a crash"
    TEST_MODULE basics.core_loads_unloads_with_content
    CONTENT "${GBA_SRAM}"
    WILL_FAIL
)