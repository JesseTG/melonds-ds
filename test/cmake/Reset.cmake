# See https://github.com/JesseTG/melonds-ds/issues/62
add_python_test(
    NAME "Core resets when using built-in system files and direct boot (NDS)"
    TEST_MODULE reset.no_hang_on_reboot
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_show_cursor=disabled"
    CORE_OPTION "melonds_sysfile_mode=builtin"
)

add_python_test(
    NAME "Core resets when using native system files and direct boot (NDS)"
    TEST_MODULE reset.no_hang_on_reboot
    CONTENT "${NDS_ROM}"
    NDS_SYSFILES
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
    CORE_OPTION "melonds_sysfile_mode=native"
)

add_python_test(
    NAME "Core resets when using native system files and native boot (NDS)"
    TEST_MODULE reset.no_hang_on_reboot
    CONTENT "${NDS_ROM}"
    NDS_SYSFILES
    CORE_OPTION "melonds_boot_mode=native"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
    CORE_OPTION "melonds_sysfile_mode=native"
)

add_python_test(
    NAME "Core resets when using native system files and direct boot (DSi)"
    TEST_MODULE reset.no_hang_on_reboot
    CONTENT "${NDS_ROM}"
    DSI_SYSFILES
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_sysfile_mode=native"
)

add_python_test(
    NAME "Core resets when using native system files and native boot (DSi)"
    TEST_MODULE reset.no_hang_on_reboot
    CONTENT "${NDS_ROM}"
    DSI_SYSFILES
    CORE_OPTION "melonds_boot_mode=native"
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_sysfile_mode=native"
)