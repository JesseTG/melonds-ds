## Direct Boot of NDS game ####################################################

add_python_test(
    NAME "Direct NDS boot with built-in system files succeeds"
    TEST_MODULE basics.core_run_frames
    CONTENT "${NDS_ROM}"
    CORE_OPTION melonds_boot_mode=direct
    CORE_OPTION melonds_console_mode=ds
    CORE_OPTION melonds_sysfile_mode=builtin
)

add_python_test(
    NAME "Direct NDS boot with native system files succeeds"
    TEST_MODULE basics.core_run_frames
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=native"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
    NDS_SYSFILES
)

add_python_test(
    NAME "Direct NDS boot with native BIOS and non-bootable firmware succeeds"
    TEST_MODULE basics.core_run_frames
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=native"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    DSI_FIRMWARE
)

## Boot to firmware ####################################################

add_python_test(
    NAME "NDS boot with no content, native BIOS, and bootable firmware succeeds"
    TEST_MODULE basics.core_run_frames
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=native"
    CORE_OPTION "melonds_boot_mode=native"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
    NDS_SYSFILES
)

add_python_test(
    NAME "NDS boot with no content and built-in system files fails"
    TEST_MODULE basics.core_run_frames
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=builtin"
    WILL_FAIL
)

add_python_test(
    NAME "NDS boot with no content, native BIOS, and non-bootable firmware fails"
    TEST_MODULE basics.core_run_frames
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=builtin"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    DSI_FIRMWARE
    WILL_FAIL
)

## DSi boot ####################################################

add_python_test(
    NAME "DSi boot to menu with no NAND image fails"
    TEST_MODULE basics.core_run_frames
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=/notfound"
    ARM7_BIOS
    ARM9_BIOS
    ARM7_DSI_BIOS
    ARM9_DSI_BIOS
    DSI_FIRMWARE
    WILL_FAIL
)

add_python_test(
    NAME "DSi boot to menu with no NDS BIOS fails"
    TEST_MODULE basics.core_run_frames
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    ARM7_DSI_BIOS
    ARM9_DSI_BIOS
    DSI_FIRMWARE
    DSI_NAND
    WILL_FAIL
)

add_python_test(
    NAME "DSi boot to menu with no DSi BIOS fails"
    TEST_MODULE basics.core_run_frames
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    DSI_FIRMWARE
    DSI_NAND
    WILL_FAIL
)

add_python_test(
    NAME "DSi boot to menu with NDS firmware fails"
    TEST_MODULE basics.core_run_frames
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${NDS_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    ARM7_DSI_BIOS
    ARM9_DSI_BIOS
    NDS_FIRMWARE
    DSI_NAND
    WILL_FAIL
)

add_python_test(
    NAME "DSi boot to menu with no firmware fails"
    TEST_MODULE basics.core_run_frames
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    ARM7_DSI_BIOS
    ARM9_DSI_BIOS
    DSI_NAND
    WILL_FAIL
)

add_python_test(
    NAME "DSi boot to menu with all system files succeeds"
    TEST_MODULE basics.core_run_frames
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    ARM7_DSI_BIOS
    ARM9_DSI_BIOS
    DSI_FIRMWARE
    DSI_NAND
)

add_python_test(
    NAME "Direct DSi boot to NDS game with no NAND image fails"
    TEST_MODULE basics.core_run_frames
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=/notfound"
    ARM7_BIOS
    ARM9_BIOS
    ARM7_DSI_BIOS
    ARM9_DSI_BIOS
    DSI_FIRMWARE
    WILL_FAIL
)

add_python_test(
    NAME "Direct DSi boot to NDS game with no NDS BIOS image fails"
    TEST_MODULE basics.core_run_frames
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    ARM7_DSI_BIOS
    ARM9_DSI_BIOS
    DSI_FIRMWARE
    DSI_NAND
    WILL_FAIL
)

add_python_test(
    NAME "Direct DSi boot to NDS game with no DSi BIOS image fails"
    TEST_MODULE basics.core_run_frames
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    DSI_FIRMWARE
    DSI_NAND
    WILL_FAIL
)

add_python_test(
    NAME "Direct DSi boot to NDS game with all system files succeeds"
    TEST_MODULE basics.core_run_frames
    CONTENT "${NDS_ROM}"
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    DSI_SYSFILES
)