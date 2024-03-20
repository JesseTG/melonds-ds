### Preventing Unneeded Loads
add_retroarch_test(
        NAME "Core doesn't try to load native BIOS if using FreeBIOS (NDS)"
        CONTENT "${NDS_ROM}"
        MAX_FRAMES 6
        CORE_OPTION "melonds_console_mode=ds"
        CORE_OPTION "melonds_sysfile_mode=builtin"
        PASS_REGULAR_EXPRESSION "Not loading native ARM BIOS files"
)



# See https://github.com/JesseTG/melonds-ds/issues/59
add_retroarch_test(
        NAME "Native firmware is not overwritten by contents of wfcsettings.bin"
        MAX_FRAMES 6
        CORE_OPTION "melonds_console_mode=ds"
        CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
        CORE_OPTION "melonds_sysfile_mode=native"
        CORE_OPTION "melonds_boot_mode=native"
        ARM7_BIOS
        ARM9_BIOS
        NDS_FIRMWARE
        REQUIRE_FILE_SIZE_UNCHANGED "system/melonDS DS/${NDS_FIRMWARE_NAME}"
)

add_retroarch_test(
        NAME "Core saves wi-fi settings to wfcsettings.bin when using built-in firmware"
        CONTENT "${NDS_ROM}"
        MAX_FRAMES 6
        CORE_OPTION "melonds_console_mode=ds"
        CORE_OPTION "melonds_firmware_nds_path=/builtin"
        ARM7_BIOS
        ARM9_BIOS
        REQUIRE_FILE_CREATED "system/melonDS DS/wfcsettings.bin"
)

### Overwriting Firmware

add_emutest_test(
        NAME "NDS firmware is not overwritten when switching from DS to DSi mode"
        CONTENT "${NDS_ROM}"
        TEST_SCRIPT "firmware-not-overwritten.lua"
        ARM7_BIOS
        ARM9_BIOS
        ARM7_DSI_BIOS
        ARM9_DSI_BIOS
        NDS_FIRMWARE
        DSI_FIRMWARE
        DSI_NAND
        CORE_OPTION melonds_firmware_dsi_path="melonDS DS/${DSI_FIRMWARE_NAME}"
        CORE_OPTION melonds_firmware_nds_path="melonDS DS/${NDS_FIRMWARE_NAME}"
        CORE_OPTION melonds_dsi_nand_path="melonDS DS/${DSI_NAND_NAME}"
        CORE_OPTION melonds_console_mode="ds"
        CORE_OPTION melonds_boot_directly="false"
        CORE_OPTION melonds_sysfile_mode="native"
        CORE_OPTION melonds_show_cursor="disabled"
)

add_emutest_test(
        NAME "NDS firmware is not overwritten when switching from DSi to NDS mode"
        CONTENT "${NDS_ROM}"
        TEST_SCRIPT "firmware-not-overwritten.lua"
        ARM7_BIOS
        ARM9_BIOS
        ARM7_DSI_BIOS
        ARM9_DSI_BIOS
        NDS_FIRMWARE
        DSI_FIRMWARE
        DSI_NAND
        CORE_OPTION melonds_firmware_dsi_path="melonDS DS/${DSI_FIRMWARE_NAME}"
        CORE_OPTION melonds_firmware_nds_path="melonDS DS/${NDS_FIRMWARE_NAME}"
        CORE_OPTION melonds_dsi_nand_path="melonDS DS/${DSI_NAND_NAME}"
        CORE_OPTION melonds_console_mode="dsi"
        CORE_OPTION melonds_boot_directly="false"
        CORE_OPTION melonds_sysfile_mode="native"
        CORE_OPTION melonds_show_cursor="disabled"
)

add_emutest_test(
        NAME "DSi firmware is not overwritten when switching from NDS to DSi mode"
        CONTENT "${NDS_ROM}"
        TEST_SCRIPT "firmware-not-overwritten.lua"
        ARM7_BIOS
        ARM9_BIOS
        ARM7_DSI_BIOS
        ARM9_DSI_BIOS
        NDS_FIRMWARE
        DSI_FIRMWARE
        DSI_NAND
        CORE_OPTION melonds_firmware_dsi_path="melonDS DS/${DSI_FIRMWARE_NAME}"
        CORE_OPTION melonds_firmware_nds_path="melonDS DS/${NDS_FIRMWARE_NAME}"
        CORE_OPTION melonds_dsi_nand_path="melonDS DS/${DSI_NAND_NAME}"
        CORE_OPTION melonds_console_mode="dsi"
        CORE_OPTION melonds_boot_directly="false"
        CORE_OPTION melonds_sysfile_mode="native"
        CORE_OPTION melonds_show_cursor="disabled"
)

add_emutest_test(
        NAME "DSi firmware is not overwritten when switching from DSi to NDS mode"
        CONTENT "${NDS_ROM}"
        TEST_SCRIPT "firmware-not-overwritten.lua"
        ARM7_BIOS
        ARM9_BIOS
        ARM7_DSI_BIOS
        ARM9_DSI_BIOS
        NDS_FIRMWARE
        DSI_FIRMWARE
        DSI_NAND
        CORE_OPTION melonds_firmware_dsi_path="melonDS DS/${DSI_FIRMWARE_NAME}"
        CORE_OPTION melonds_firmware_nds_path="melonDS DS/${NDS_FIRMWARE_NAME}"
        CORE_OPTION melonds_dsi_nand_path="melonDS DS/${DSI_NAND_NAME}"
        CORE_OPTION melonds_console_mode="dsi"
        CORE_OPTION melonds_boot_directly="false"
        CORE_OPTION melonds_sysfile_mode="native"
        CORE_OPTION melonds_show_cursor="disabled"
)