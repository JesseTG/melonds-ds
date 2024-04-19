### Preventing Unneeded Loads
add_python_test(
    NAME "Core doesn't try to load native BIOS if using FreeBIOS (NDS)"
    TEST_MODULE firmware.native_bios_not_loaded_with_freebios
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=builtin"
)

### Ensuring firmware is not overwritten

# See https://github.com/JesseTG/melonds-ds/issues/59
add_python_test(
    NAME "Native firmware is not overwritten by contents of wfcsettings.bin"
    TEST_MODULE firmware.nds_firmware_not_overwritten
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
    CORE_OPTION "melonds_sysfile_mode=native"
    CORE_OPTION "melonds_boot_mode=native"
    NDS_SYSFILES
)

add_python_test(
    NAME "Core saves wi-fi settings to wfcsettings.bin when using built-in firmware"
    TEST_MODULE firmware.core_saves_wfcsettings
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_firmware_nds_path=/builtin"
    CORE_OPTION "melonds_sysfile_mode=builtin"
)

add_python_test(
    NAME "Firmware images aren't overwritten when switching from DS to DSi mode"
    TEST_MODULE firmware.firmware_not_overwritten_when_switching_console_mode
    CONTENT "${NDS_ROM}"
    NDS_SYSFILES
    DSI_SYSFILES
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_boot_directly=false"
    CORE_OPTION "melonds_sysfile_mode=native"
)

add_python_test(
    NAME "Firmware images aren't overwritten when switching from DSi to DS mode"
    TEST_MODULE firmware.firmware_not_overwritten_when_switching_console_mode
    CONTENT "${NDS_ROM}"
    NDS_SYSFILES
    DSI_SYSFILES
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_boot_directly=false"
    CORE_OPTION "melonds_sysfile_mode=native"
)
