### With Content

add_retroarch_test(
        NAME "Core falls back to FreeBIOS if ARM7 BIOS is missing (NDS)"
        CONTENT "${NDS_ROM}"
        MAX_FRAMES 6
        CORE_OPTION "melonds_console_mode=ds"
        CORE_OPTION "melonds_sysfile_mode=native"
        CORE_OPTION "melonds_boot_mode=direct"
        ARM9_BIOS
        PASS_REGULAR_EXPRESSION "Falling back to FreeBIOS"
)

add_retroarch_test(
        NAME "Core falls back to FreeBIOS if ARM9 BIOS is missing (NDS)"
        CONTENT "${NDS_ROM}"
        MAX_FRAMES 6
        CORE_OPTION "melonds_console_mode=ds"
        CORE_OPTION "melonds_sysfile_mode=native"
        CORE_OPTION "melonds_boot_mode=direct"
        ARM7_BIOS
        PASS_REGULAR_EXPRESSION "Falling back to FreeBIOS"
)

add_retroarch_test(
        NAME "Core falls back to built-in firmware if native firmware is missing (NDS)"
        CONTENT "${NDS_ROM}"
        MAX_FRAMES 6
        CORE_OPTION "melonds_console_mode=ds"
        CORE_OPTION "melonds_sysfile_mode=native"
        CORE_OPTION "melonds_boot_mode=direct"
        CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
        ARM7_BIOS
        ARM9_BIOS
        PASS_REGULAR_EXPRESSION "Falling back to built-in firmware"
)

add_emutest_test(
        NAME "Loading invalid ROM does not cause a crash"
        CONTENT "$<TARGET_FILE:melondsds_libretro>"
        TEST_SCRIPT "no-crash-on-invalid-rom.lua"
        WILL_FAIL
)
