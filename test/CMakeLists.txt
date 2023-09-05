find_package(Python3 COMPONENTS Interpreter)

if (NOT RETROARCH)
    find_program(RETROARCH retroarch)
endif()
message(STATUS "RETROARCH: ${RETROARCH}")

cmake_path(GET RETROARCH PARENT_PATH RETROARCH_DIR)

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

    message(STATUS "${BIOS_NAME}: ${${BIOS_NAME}} (${BIOS_SIZE} bytes, MD5=${BIOS_MD5})")
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

include(CMakePrintHelpers)

function(add_retroarch_test)
    set(options WILL_FAIL ARM7_BIOS ARM9_BIOS ARM7_DSI_BIOS ARM9_DSI_BIOS NDS_FIRMWARE DSI_FIRMWARE DSI_NAND)
    set(oneValueArgs NAME MAX_FRAMES CONTENT)
    set(multiValueArgs CORE_OPTION PASS_REGULAR_EXPRESSION FAIL_REGULAR_EXPRESSION SKIP_REGULAR_EXPRESSION)
    cmake_parse_arguments(PARSE_ARGV 0 RETRO "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if (NOT RETRO_MAX_FRAMES)
        set(RETRO_MAX_FRAMES 30)
    endif()

    add_test(
        NAME "${RETRO_NAME}"
        COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/python/retroarch.py"
            --libretro "$<TARGET_FILE:libretro>"
            --max-frames=${RETRO_MAX_FRAMES}
            "${RETRO_CONTENT}"
    )

    list(APPEND REQUIRED_FILES "$<TARGET_FILE:libretro>")
    if (RETRO_CONTENT)
        list(APPEND REQUIRED_FILES "${RETRO_CONTENT}")
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
endfunction()

add_retroarch_test(
    NAME "RetroArch loads melonDS DS"
    CONTENT "${CMAKE_BINARY_DIR}/systemTest.nds"
)

# See https://github.com/JesseTG/melonds-ds/issues/51
add_retroarch_test(
    NAME "Core exposes config options to frontend"
    CONTENT "${CMAKE_BINARY_DIR}/systemTest.nds"
    FAIL_REGULAR_EXPRESSION "GET_VARIABLE: melonds_[a-z_]+ - Not implemented"
)

### Preventing Unneeded Loads

add_retroarch_test(
    NAME "Core doesn't try to load native firmware if using built-in firmware (NDS)"
    CONTENT "${CMAKE_BINARY_DIR}/mkds.nds"
    MAX_FRAMES 6
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=disabled"
    CORE_OPTION "melonds_firmware_nds_path=/builtin"
    FAIL_REGULAR_EXPRESSION "[\\/]/builtin"
    # "/builtin" is the value used to indicate the built-in firmware
    # chosen because it's unlikely to be a valid path
)

add_retroarch_test(
    NAME "Core doesn't try to load native BIOS if using FreeBIOS (NDS)"
    CONTENT "${CMAKE_BINARY_DIR}/mkds.nds"
    MAX_FRAMES 6
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=disabled"
    CORE_OPTION "melonds_firmware_nds_path=/builtin"
    PASS_REGULAR_EXPRESSION "Not loading native ARM BIOS files"
)

### With Content

add_retroarch_test(
    NAME "Core falls back to FreeBIOS if ARM7 BIOS is missing (NDS)"
    CONTENT "${CMAKE_BINARY_DIR}/mkds.nds"
    MAX_FRAMES 6
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=enabled"
    CORE_OPTION "melonds_boot_directly=enabled"
    CORE_OPTION "melonds_firmware_nds_path=/builtin"
    ARM9_BIOS
    PASS_REGULAR_EXPRESSION "Falling back to FreeBIOS"
)

add_retroarch_test(
    NAME "Core falls back to FreeBIOS if ARM9 BIOS is missing (NDS)"
    CONTENT "${CMAKE_BINARY_DIR}/mkds.nds"
    MAX_FRAMES 6
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=enabled"
    CORE_OPTION "melonds_boot_directly=enabled"
    CORE_OPTION "melonds_firmware_nds_path=/builtin"
    ARM7_BIOS
    PASS_REGULAR_EXPRESSION "Falling back to FreeBIOS"
)

add_retroarch_test(
    NAME "Core falls back to built-in firmware if native firmware is missing (NDS)"
    CONTENT "${CMAKE_BINARY_DIR}/mkds.nds"
    MAX_FRAMES 6
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=enabled"
    CORE_OPTION "melonds_boot_directly=enabled"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/firmware.bin"
    ARM7_BIOS
    ARM9_BIOS
    PASS_REGULAR_EXPRESSION "Installed firmware \\(Identifier: MELN\\)"
)


## Direct Boot of NDS game ####################################################

add_retroarch_test(
    NAME "Direct NDS boot with FreeBIOS and built-in firmware succeeds"
    CONTENT "${CMAKE_BINARY_DIR}/mkds.nds"
    MAX_FRAMES 180
    CORE_OPTION "melonds_boot_directly=enabled"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=disabled"
    CORE_OPTION "melonds_firmware_nds_path=/builtin"
)

add_retroarch_test(
    NAME "Direct NDS boot with native BIOS and native firmware succeeds"
    CONTENT "${CMAKE_BINARY_DIR}/mkds.nds"
    MAX_FRAMES 180
    CORE_OPTION "melonds_boot_directly=enabled"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=enabled"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/firmware.bin"
    ARM7_BIOS
    ARM9_BIOS
    NDS_FIRMWARE
    FAIL_REGULAR_EXPRESSION "Failed to load ARM[79]"
    FAIL_REGULAR_EXPRESSION "Failed to load the required firmware"
)

add_retroarch_test(
    NAME "Direct NDS boot with native BIOS and non-bootable firmware succeeds"
    CONTENT "${CMAKE_BINARY_DIR}/mkds.nds"
    MAX_FRAMES 180
    CORE_OPTION "melonds_boot_directly=enabled"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=enabled"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/dsi_firmware.bin"
    ARM7_BIOS
    ARM9_BIOS
    DSI_FIRMWARE
    FAIL_REGULAR_EXPRESSION "Failed to load ARM[79]"
    FAIL_REGULAR_EXPRESSION "Failed to load the required firmware"
)

add_retroarch_test(
    NAME "Direct NDS boot with native BIOS and built-in firmware succeeds"
    CONTENT "${CMAKE_BINARY_DIR}/mkds.nds"
    MAX_FRAMES 180
    CORE_OPTION "melonds_boot_directly=enabled"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=enabled"
    CORE_OPTION "melonds_firmware_nds_path=/builtin"
    ARM7_BIOS
    ARM9_BIOS
    FAIL_REGULAR_EXPRESSION "Failed to load the required firmware"
)

add_retroarch_test(
    NAME "Direct NDS boot with FreeBIOS and native firmware succeeds"
    CONTENT "${CMAKE_BINARY_DIR}/mkds.nds"
    MAX_FRAMES 180
    CORE_OPTION "melonds_boot_directly=enabled"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=disabled"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/firmware.bin"
    NDS_FIRMWARE
    FAIL_REGULAR_EXPRESSION "Failed to load the required firmware"
)

## Boot to firmware ####################################################

add_retroarch_test(
    NAME "Native NDS boot with no content, FreeBIOS, and built-in firmware fails"
    MAX_FRAMES 180
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=disabled"
    CORE_OPTION "melonds_firmware_nds_path=/builtin"
    WILL_FAIL
)

add_retroarch_test(
    NAME "NDS boot with no content, FreeBIOS, and native firmware fails"
    MAX_FRAMES 180
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=disabled"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/firmware.bin"
    NDS_FIRMWARE
    WILL_FAIL
)

add_retroarch_test(
    NAME "NDS boot with no content, native BIOS, and built-in firmware fails"
    MAX_FRAMES 180
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=enabled"
    ARM7_BIOS
    ARM9_BIOS
    WILL_FAIL
)

add_retroarch_test(
    NAME "NDS boot with no content, native BIOS, and non-bootable firmware fails"
    MAX_FRAMES 180
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=disabled"
    CORE_OPTION "melonds_firmware_nds_path=melonDS DS/dsi_firmware.bin"
    ARM7_BIOS
    ARM9_BIOS
    DSI_FIRMWARE
    WILL_FAIL
)

# TODO: Write the following test capabilities:
# - Copying a system file to a particular location
# - Select a GBA ROM
# - Select a GBA save file

# TODO: Write the following tests:

# - Core loads firmware from system
# - Core loads firmware from system/melonDS DS
# - Core loads BIOS from system
# - Core loads BIOS from system/melonDS DS
# - Core loads NDS game with GBA game in slot-2
# - Core loads NDS game with GBA game in slot-2 and save data
# - Core loads NDS game with GBA game in slot-2 and save data, and can save GBA data
# (will need to find a test case besides Pokemon)