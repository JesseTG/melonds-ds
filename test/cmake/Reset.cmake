
# See https://github.com/JesseTG/melonds-ds/issues/62
add_emutest_test(
        NAME "Core resets when using built-in system files and direct boot (NDS)"
        CONTENT "${NDS_ROM}"
        TEST_SCRIPT "no-hang-on-reboot.lua"
        CORE_OPTION melonds_boot_mode="direct"
        CORE_OPTION melonds_show_cursor="disabled"
        CORE_OPTION melonds_sysfile_mode="builtin"
        CORE_OPTION melonds_console_mode="ds"
)

add_emutest_test(
        NAME "Core resets when using native system files and direct boot (NDS)"
        CONTENT "${NDS_ROM}"
        TEST_SCRIPT "no-hang-on-reboot.lua"
        ARM7_BIOS
        ARM9_BIOS
        NDS_FIRMWARE
        CORE_OPTION melonds_firmware_nds_path='melonDS DS/${NDS_FIRMWARE_NAME}'
        CORE_OPTION melonds_boot_mode="direct"
        CORE_OPTION melonds_sysfile_mode="native"
        CORE_OPTION melonds_show_cursor="disabled"
        CORE_OPTION melonds_console_mode="ds"
)

add_emutest_test(
        NAME "Core resets when using native system files and native boot (NDS)"
        CONTENT "${NDS_ROM}"
        TEST_SCRIPT "no-hang-on-reboot.lua"
        ARM7_BIOS
        ARM9_BIOS
        NDS_FIRMWARE
        CORE_OPTION melonds_firmware_nds_path="melonDS DS/${NDS_FIRMWARE_NAME}"
        CORE_OPTION melonds_boot_mode="native"
        CORE_OPTION melonds_sysfile_mode="native"
        CORE_OPTION melonds_show_cursor="disabled"
)

add_emutest_test(
        NAME "Core resets when using native system files and direct boot (DSi)"
        CONTENT "${NDS_ROM}"
        TEST_SCRIPT "no-hang-on-reboot.lua"
        ARM7_BIOS
        ARM9_BIOS
        ARM7_DSI_BIOS
        ARM9_DSI_BIOS
        DSI_FIRMWARE
        DSI_NAND
        CORE_OPTION melonds_firmware_dsi_path="melonDS DS/${DSI_FIRMWARE_NAME}"
        CORE_OPTION melonds_dsi_nand_path="melonDS DS/${DSI_NAND_NAME}"
        CORE_OPTION melonds_console_mode="dsi"
        CORE_OPTION melonds_boot_mode="direct"
        CORE_OPTION melonds_sysfile_mode="native"
        CORE_OPTION melonds_show_cursor="disabled"
)

add_emutest_test(
        NAME "Core resets when using native system files and native boot (DSi)"
        CONTENT "${NDS_ROM}"
        TEST_SCRIPT "no-hang-on-reboot.lua"
        ARM7_BIOS
        ARM9_BIOS
        ARM7_DSI_BIOS
        ARM9_DSI_BIOS
        DSI_FIRMWARE
        DSI_NAND
        CORE_OPTION melonds_firmware_dsi_path="melonDS DS/${DSI_FIRMWARE_NAME}"
        CORE_OPTION melonds_console_mode="dsi"
        CORE_OPTION melonds_dsi_nand_path="melonDS DS/${DSI_NAND_NAME}"
        CORE_OPTION melonds_boot_mode="native"
        CORE_OPTION melonds_sysfile_mode="native"
        CORE_OPTION melonds_show_cursor="disabled"
)