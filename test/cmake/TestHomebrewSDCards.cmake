add_python_test(
    NAME "Homebrew SD card image is not created if loading a retail ROM"
    CONTENT "${NDS_ROM}"
    TEST_MODULE save.homebrew_sd_card_exists
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_homebrew_sdcard=enabled"
    CORE_OPTION "melonds_homebrew_sync_sdcard_to_host=disabled"
    WILL_FAIL
)

add_python_test(
    NAME "Homebrew SD card image is not created if disabled and loading a retail ROM"
    CONTENT "${NDS_ROM}"
    TEST_MODULE save.homebrew_sd_card_exists
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_homebrew_sdcard=disabled"
    CORE_OPTION "melonds_homebrew_sync_sdcard_to_host=disabled"
    WILL_FAIL
)

add_python_test(
    NAME "Homebrew SD card image is created if enabled and loading a homebrew ROM"
    CONTENT "${GODMODE9I_ROM}"
    TEST_MODULE save.homebrew_sd_card_exists
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_homebrew_sdcard=enabled"
    CORE_OPTION "melonds_homebrew_sync_sdcard_to_host=disabled"
)

add_python_test(
    NAME "Homebrew SD card image is not created if disabled and loading a homebrew ROM"
    CONTENT "${GODMODE9I_ROM}"
    TEST_MODULE save.homebrew_sd_card_exists
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_homebrew_sdcard=disabled"
    CORE_OPTION "melonds_homebrew_sync_sdcard_to_host=disabled"
    WILL_FAIL
)

add_python_test(
    NAME "Homebrew SD card sync folder is not created if SD card is enabled and loading a retail ROM"
    CONTENT "${NDS_ROM}"
    TEST_MODULE save.homebrew_sd_card_sync_exists
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_homebrew_sdcard=enabled"
    CORE_OPTION "melonds_homebrew_sync_sdcard_to_host=enabled"
    WILL_FAIL
)

add_python_test(
    NAME "Homebrew SD card sync folder is not created if SD card sync is disabled and loading a retail ROM"
    CONTENT "${NDS_ROM}"
    TEST_MODULE save.homebrew_sd_card_sync_exists
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_homebrew_sdcard=enabled"
    CORE_OPTION "melonds_homebrew_sync_sdcard_to_host=disabled"
    WILL_FAIL
)

add_python_test(
    NAME "Homebrew SD card sync folder is not created if SD card sync is disabled and loading a homebrew ROM"
    CONTENT "${GODMODE9I_ROM}"
    TEST_MODULE save.homebrew_sd_card_sync_exists
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_homebrew_sdcard=enabled"
    CORE_OPTION "melonds_homebrew_sync_sdcard_to_host=disabled"
    WILL_FAIL
)