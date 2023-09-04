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

add_retroarch_test(
    NAME "melonDS DS loads retail game with generated BIOS and firmware in NDS mode"
    CONTENT "${CMAKE_BINARY_DIR}/mkds.nds"
    CORE_OPTION "melonds_console_mode=ds"
    CORE_OPTION "melonds_use_external_bios=disabled"
    CORE_OPTION "melonds_firmware_nds_path=/builtin"
)

