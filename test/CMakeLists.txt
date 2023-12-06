find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)

if (NOT RETROARCH)
    find_program(RETROARCH retroarch REQUIRED)
endif()
message(STATUS "RETROARCH: ${RETROARCH}")

if (NOT EMUTEST)
    find_program(EMUTEST emutest REQUIRED)
endif()
message(STATUS "EMUTEST: ${EMUTEST}")

cmake_path(GET RETROARCH PARENT_PATH RETROARCH_DIR)

if (NOT NDS_ROM)
    message(WARNING "NDS_ROM must be set to the path of an NDS ROM")
    set(NDS_ROM "NDS_ROM-NOTFOUND" CACHE FILEPATH "Path to an NDS ROM" FORCE)
else()
    message(DEBUG "NDS_ROM: ${NDS_ROM}")
endif()

function(find_bios BIOS_NAME BIOS_FILE EXPECTED_BIOS_SIZES KNOWN_BIOS_HASHES)
    if (NOT ${BIOS_NAME})
        find_file(${BIOS_NAME} ${BIOS_FILE} HINTS "${RETROARCH_DIR}/system" REQUIRED)
    endif()

    file(SIZE ${${BIOS_NAME}} BIOS_SIZE)
    if (NOT (BIOS_SIZE IN_LIST EXPECTED_BIOS_SIZES))
        message(SEND_ERROR "Expected ${BIOS_NAME} with size in ${EXPECTED_BIOS_SIZES}, got ${BIOS_SIZE}")
    endif()

    file(MD5 ${${BIOS_NAME}} BIOS_MD5)
    string(TOLOWER "${BIOS_MD5}" BIOS_MD5)
    if (DEFINED ${KNOWN_BIOS_HASHES})
        if (NOT (BIOS_MD5 IN_LIST KNOWN_BIOS_HASHES))
            message(SEND_ERROR "Expected ${BIOS_NAME} with an MD5 in ${KNOWN_BIOS_HASHES}, got \"${BIOS_MD5}\"")
        endif()
    endif()

    message(DEBUG "${BIOS_NAME}: ${${BIOS_NAME}} (${BIOS_SIZE} bytes, MD5=${BIOS_MD5})")
endfunction()


list(APPEND ARM7_BIOS_HASHES "df692a80a5b1bc90728bc3dfc76cd948") # NDS ARM7 BIOS
find_bios(ARM7_BIOS bios7.bin 16384 ${ARM7_BIOS_HASHES})

list(APPEND ARM9_BIOS_HASHES "a392174eb3e572fed6447e956bde4b25") # NDS ARM9 BIOS (World)
list(APPEND ARM9_BIOS_HASHES "08f1dc42ebe85f129b06415f5e2ccbc8") # NDS ARM9 BIOS (China)
find_bios(ARM9_BIOS bios9.bin 4096 ${ARM9_BIOS_HASHES})

find_bios(ARM7_DSI_BIOS dsi_bios7.bin 65536 ARM7_DSI_BIOS_HASHES)
find_bios(ARM9_DSI_BIOS dsi_bios9.bin 65536 ARM9_DSI_BIOS_HASHES)

find_bios(NDS_FIRMWARE firmware.bin "131072;262144;524288" NDS_FIRMWARE_HASHES)
find_bios(DSI_FIRMWARE dsi_firmware.bin "131072;262144;524288" DSI_FIRMWARE_HASHES)
find_bios(DSI_NAND dsi_nand.bin 251658304 DSI_NAND_HASHES)

cmake_path(GET ARM7_BIOS FILENAME ARM7_BIOS_NAME)
cmake_path(GET ARM9_BIOS FILENAME ARM9_BIOS_NAME)
cmake_path(GET ARM7_DSI_BIOS FILENAME ARM7_DSI_BIOS_NAME)
cmake_path(GET ARM9_DSI_BIOS FILENAME ARM9_DSI_BIOS_NAME)
cmake_path(GET NDS_FIRMWARE FILENAME NDS_FIRMWARE_NAME)
cmake_path(GET DSI_FIRMWARE FILENAME DSI_FIRMWARE_NAME)
cmake_path(GET DSI_NAND FILENAME DSI_NAND_NAME)

include(CMakePrintHelpers)

function(add_retroarch_test)
    set(options WILL_FAIL ARM7_BIOS ARM9_BIOS ARM7_DSI_BIOS ARM9_DSI_BIOS NDS_FIRMWARE DSI_FIRMWARE DSI_NAND NO_SKIP_ERROR_SCREEN DISABLED)
    set(oneValueArgs NAME MAX_FRAMES CONTENT REQUIRE_FILE_SIZE_UNCHANGED REQUIRE_FILE_CREATED)
    set(multiValueArgs CORE_OPTION PASS_REGULAR_EXPRESSION FAIL_REGULAR_EXPRESSION SKIP_REGULAR_EXPRESSION)
    cmake_parse_arguments(PARSE_ARGV 0 RETRO "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if (NOT RETRO_MAX_FRAMES)
        set(RETRO_MAX_FRAMES 30)
    endif()

    add_test(
        NAME "${RETRO_NAME}"
        COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/python/retroarch.py"
            --libretro "$<TARGET_FILE:melondsds_libretro>"
            --max-frames=${RETRO_MAX_FRAMES}
            "${RETRO_CONTENT}"
    )

    list(APPEND REQUIRED_FILES "$<TARGET_FILE:melondsds_libretro>")
    if (RETRO_CONTENT)
        list(APPEND REQUIRED_FILES "${RETRO_CONTENT}")
    endif()

    if (NOT RETRO_NO_SKIP_ERROR_SCREEN)
        list(APPEND ENVIRONMENT MELONDSDS_SKIP_ERROR_SCREEN=1)
    endif()

    if (RETRO_REQUIRE_FILE_SIZE_UNCHANGED)
        list(APPEND ENVIRONMENT "REQUIRE_FILE_SIZE_UNCHANGED=${RETRO_REQUIRE_FILE_SIZE_UNCHANGED}")
    endif()

    if (RETRO_REQUIRE_FILE_CREATED)
        list(APPEND ENVIRONMENT "REQUIRE_FILE_CREATED=${RETRO_REQUIRE_FILE_CREATED}")
    endif()

    list(APPEND ENVIRONMENT "RETROARCH=${RETROARCH}")
    list(APPEND ENVIRONMENT ${RETRO_CORE_OPTION}) # Not an omission, this is already a list

    macro(expose_system_file SYSFILE)
        if (RETRO_${SYSFILE})
            list(APPEND REQUIRED_FILES "${${SYSFILE}}")
            list(APPEND ENVIRONMENT "${SYSFILE}=${${SYSFILE}}")
        endif()
    endmacro()

    expose_system_file(ARM7_BIOS)
    expose_system_file(ARM9_BIOS)
    expose_system_file(ARM7_DSI_BIOS)
    expose_system_file(ARM9_DSI_BIOS)
    expose_system_file(NDS_FIRMWARE)
    expose_system_file(DSI_FIRMWARE)
    expose_system_file(DSI_NAND)

    set_tests_properties("${RETRO_NAME}" PROPERTIES LABELS "RetroArch")
    set_tests_properties("${RETRO_NAME}" PROPERTIES ENVIRONMENT "${ENVIRONMENT}")
    set_tests_properties("${RETRO_NAME}" PROPERTIES REQUIRED_FILES "${REQUIRED_FILES}")
    if (RETRO_PASS_REGULAR_EXPRESSION)
        set_tests_properties("${RETRO_NAME}" PROPERTIES PASS_REGULAR_EXPRESSION "${RETRO_PASS_REGULAR_EXPRESSION}")
    endif()

    if (RETRO_FAIL_REGULAR_EXPRESSION)
        set_tests_properties("${RETRO_NAME}" PROPERTIES FAIL_REGULAR_EXPRESSION "${RETRO_FAIL_REGULAR_EXPRESSION}")
    endif()

    if (RETRO_SKIP_REGULAR_EXPRESSION)
        set_tests_properties("${RETRO_NAME}" PROPERTIES SKIP_REGULAR_EXPRESSION "${RETRO_SKIP_REGULAR_EXPRESSION}")
    endif()

    if (RETRO_WILL_FAIL)
        set_tests_properties("${RETRO_NAME}" PROPERTIES WILL_FAIL TRUE)
    endif()

    if (RETRO_DISABLED)
        set_tests_properties("${RETRO_NAME}" PROPERTIES DISABLED TRUE)
    endif()
endfunction()

function(add_emutest_test)
    set(options WILL_FAIL ARM7_BIOS ARM9_BIOS ARM7_DSI_BIOS ARM9_DSI_BIOS NDS_FIRMWARE DSI_FIRMWARE DSI_NAND NO_SKIP_ERROR_SCREEN DISABLED)
    set(oneValueArgs NAME TEST_SCRIPT CONTENT)
    set(multiValueArgs CORE_OPTION PASS_REGULAR_EXPRESSION FAIL_REGULAR_EXPRESSION SKIP_REGULAR_EXPRESSION)
    cmake_parse_arguments(PARSE_ARGV 0 RETRO "${options}" "${oneValueArgs}" "${multiValueArgs}")

    add_test(
        NAME "${RETRO_NAME}"
        COMMAND ${EMUTEST}
        -L "$<TARGET_FILE:melondsds_libretro>"
        -r "${RETRO_CONTENT}"
        -t "${CMAKE_CURRENT_SOURCE_DIR}/lua/${RETRO_TEST_SCRIPT}"
    )

    list(APPEND REQUIRED_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lua/preamble.lua")
    list(APPEND REQUIRED_FILES "$<TARGET_FILE:melondsds_libretro>")
    if (RETRO_CONTENT)
        list(APPEND REQUIRED_FILES "${RETRO_CONTENT}")
    endif()


    if (WIN32)
        cmake_path(CONVERT "$ENV{USERPROFILE}\\.emutest" TO_CMAKE_PATH_LIST EMUTEST_HOME)
        # emutest hardcodes a single root directory
    else()
        set(EMUTEST_HOME "$ENV{HOME}/.emutest")
    endif()
    if (NOT RETRO_NO_SKIP_ERROR_SCREEN)
        list(APPEND ENVIRONMENT MELONDSDS_SKIP_ERROR_SCREEN=1)
    endif()

    list(APPEND ENVIRONMENT "EMUTEST_HOME=${EMUTEST_HOME}")
    list(APPEND ENVIRONMENT "PREAMBLE=${CMAKE_CURRENT_SOURCE_DIR}/lua/preamble.lua")
    list(JOIN RETRO_CORE_OPTION "\n" RETRO_CORE_OPTIONS)
    list(APPEND ENVIRONMENT "RETRO_CORE_OPTIONS=${RETRO_CORE_OPTIONS}")

    macro(expose_system_file SYSFILE)
        if (RETRO_${SYSFILE})
            cmake_path(GET ${SYSFILE} FILENAME filename)
            # emutest doesn't use standard Lua; it uses a Go port that happens to not support regexes.
            # So we have to pass in the basename separately.
            list(APPEND REQUIRED_FILES "${${SYSFILE}}")
            list(APPEND ENVIRONMENT "${SYSFILE}=${${SYSFILE}}")
            list(APPEND ENVIRONMENT "${SYSFILE}_NAME=${filename}")
        endif()
    endmacro()

    expose_system_file(ARM7_BIOS)
    expose_system_file(ARM9_BIOS)
    expose_system_file(ARM7_DSI_BIOS)
    expose_system_file(ARM9_DSI_BIOS)
    expose_system_file(NDS_FIRMWARE)
    expose_system_file(DSI_FIRMWARE)
    expose_system_file(DSI_NAND)

    set_tests_properties("${RETRO_NAME}" PROPERTIES LABELS "emutest")
    set_tests_properties("${RETRO_NAME}" PROPERTIES ENVIRONMENT "${ENVIRONMENT}")
    set_tests_properties("${RETRO_NAME}" PROPERTIES REQUIRED_FILES "${REQUIRED_FILES}")
    set_tests_properties("${RETRO_NAME}" PROPERTIES TIMEOUT 30)

    # emutest only supports a pre-defined base directory, so we can only run one emutest test at a time
    set_tests_properties("${RETRO_NAME}" PROPERTIES RESOURCE_LOCK "homedir")
    if (RETRO_PASS_REGULAR_EXPRESSION)
        set_tests_properties("${RETRO_NAME}" PROPERTIES PASS_REGULAR_EXPRESSION "${RETRO_PASS_REGULAR_EXPRESSION}")
    endif()

    if (RETRO_FAIL_REGULAR_EXPRESSION)
        set_tests_properties("${RETRO_NAME}" PROPERTIES FAIL_REGULAR_EXPRESSION "${RETRO_FAIL_REGULAR_EXPRESSION}")
    endif()

    if (RETRO_SKIP_REGULAR_EXPRESSION)
        set_tests_properties("${RETRO_NAME}" PROPERTIES SKIP_REGULAR_EXPRESSION "${RETRO_SKIP_REGULAR_EXPRESSION}")
    endif()

    if (RETRO_WILL_FAIL)
        set_tests_properties("${RETRO_NAME}" PROPERTIES WILL_FAIL TRUE)
    endif()

    if (RETRO_DISABLED)
        set_tests_properties("${RETRO_NAME}" PROPERTIES DISABLED TRUE)
    endif()
endfunction()

add_retroarch_test(
    NAME "RetroArch loads melonDS DS"
    CONTENT "${NDS_ROM}"
)

add_emutest_test(
    NAME "emutest loads melonDS DS"
    CONTENT "${NDS_ROM}"
    TEST_SCRIPT "loads-core.lua"
)

# See https://github.com/JesseTG/melonds-ds/issues/70
add_retroarch_test(
    NAME "Core unloads with threaded software rendering"
    CONTENT "${NDS_ROM}"
    MAX_FRAMES 6
    DISABLED
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_sysfile_mode=builtin"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_threaded_renderer=enabled"
)

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

# See https://github.com/JesseTG/melonds-ds/issues/51
add_retroarch_test(
    NAME "Core exposes config options to frontend"
    CONTENT "${NDS_ROM}"
    FAIL_REGULAR_EXPRESSION "GET_VARIABLE: melonds_[a-z_]+ - Not implemented"
)

### Preventing Unneeded Loads

add_retroarch_test(
    NAME "Core doesn't try to load native BIOS if using FreeBIOS (NDS)"
    CONTENT "${NDS_ROM}"
    MAX_FRAMES 6
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=builtin"
    PASS_REGULAR_EXPRESSION "Not loading native ARM BIOS files"
)

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
    PASS_REGULAR_EXPRESSION "Installed firmware \\(Identifier: MELN\\)"
)


## Direct Boot of NDS game ####################################################

add_retroarch_test(
    NAME "Direct NDS boot with built-in system files succeeds"
    CONTENT "${NDS_ROM}"
    MAX_FRAMES 180
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=builtin"
)

add_retroarch_test(
    NAME "Direct NDS boot with native system files succeeds"
    CONTENT "${NDS_ROM}"
    MAX_FRAMES 180
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=native"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    NDS_FIRMWARE
    FAIL_REGULAR_EXPRESSION "Failed to load ARM[79]"
    FAIL_REGULAR_EXPRESSION "Failed to load the required firmware"
)

add_retroarch_test(
    NAME "Direct NDS boot with native BIOS and non-bootable firmware succeeds"
    CONTENT "${NDS_ROM}"
    MAX_FRAMES 180
    CORE_OPTION "melonds_boot_mode=direct"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=native"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    DSI_FIRMWARE
    FAIL_REGULAR_EXPRESSION "Failed to load ARM[79]"
    FAIL_REGULAR_EXPRESSION "Failed to load the required firmware"
)

## Boot to firmware ####################################################

add_retroarch_test(
    NAME "NDS boot with no content, native BIOS, and bootable firmware succeeds"
    MAX_FRAMES 180
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=native"
    CORE_OPTION "melonds_boot_mode=native"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${NDS_FIRMWARE_NAME}"
    NDS_FIRMWARE
    ARM7_BIOS
    ARM9_BIOS
)

add_retroarch_test(
    NAME "NDS boot with no content and built-in system files fails"
    MAX_FRAMES 180
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=builtin"
    WILL_FAIL
)

add_retroarch_test(
    NAME "NDS boot with no content, native BIOS, and non-bootable firmware fails"
    MAX_FRAMES 180
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_sysfile_mode=builtin"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    DSI_FIRMWARE
    WILL_FAIL
)

## DSi boot ####################################################

add_retroarch_test(
    NAME "DSi boot to menu with no NAND image fails"
    MAX_FRAMES 6
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

add_retroarch_test(
    NAME "DSi boot to menu with no NDS BIOS fails"
    MAX_FRAMES 6
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    ARM7_DSI_BIOS
    ARM9_DSI_BIOS
    DSI_FIRMWARE
    DSI_NAND
    WILL_FAIL
)

add_retroarch_test(
    NAME "DSi boot to menu with no DSi BIOS fails"
    MAX_FRAMES 6
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_firmware_dsi_path=melonDS DS/${DSI_FIRMWARE_NAME}"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    DSI_FIRMWARE
    DSI_NAND
    WILL_FAIL
)

add_retroarch_test(
    NAME "DSi boot to menu with NDS firmware fails"
    MAX_FRAMES 6
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

add_retroarch_test(
    NAME "DSi boot to menu with no firmware fails"
    MAX_FRAMES 6
    CORE_OPTION "melonds_console_mode=dsi"
    CORE_OPTION "melonds_dsi_nand_path=melonDS DS/${DSI_NAND_NAME}"
    ARM7_BIOS
    ARM9_BIOS
    ARM7_DSI_BIOS
    ARM9_DSI_BIOS
    DSI_NAND
    WILL_FAIL
)

add_retroarch_test(
    NAME "DSi boot to menu with all system files succeeds"
    MAX_FRAMES 6
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

add_retroarch_test(
    NAME "Direct DSi boot to NDS game with no NAND image fails"
    CONTENT "${NDS_ROM}"
    MAX_FRAMES 6
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

add_retroarch_test(
    NAME "Direct DSi boot to NDS game with no NDS BIOS image fails"
    CONTENT "${NDS_ROM}"
    MAX_FRAMES 6
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

add_retroarch_test(
    NAME "Direct DSi boot to NDS game with no DSi BIOS image fails"
    CONTENT "${NDS_ROM}"
    MAX_FRAMES 6
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

add_retroarch_test(
    NAME "Direct DSi boot to NDS game with all system files succeeds"
    CONTENT "${NDS_ROM}"
    MAX_FRAMES 6
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

### Using wfcsettings.bin

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

# TODO: Write the following test capabilities:
# - Copying a system file to a particular location
# - Select a GBA ROM
# - Select a GBA save file

# TODO: Write the following tests:

# - Core loads BIOS from system
# - Core loads BIOS from system/melonDS DS
# - Core loads NDS game with GBA game in slot-2
# - Core loads NDS game with GBA game in slot-2 and save data
# - Core loads NDS game with GBA game in slot-2 and save data, and can save GBA data
# (will need to find a test case besides Pokemon)