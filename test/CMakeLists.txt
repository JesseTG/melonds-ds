find_package(Python 3.11 REQUIRED)

# FindPython sets "Python_EXECUTABLE", but FindPythonModules wants "PYTHON_EXECUTABLE" (note the caps)
set(PYTHON_EXECUTABLE "${Python_EXECUTABLE}")

find_package(PythonModules COMPONENTS libretro PIL)

# TODO: Make this required once libretro.py is on PyPI
if (PythonModules_libretro_PATH)
    message(STATUS "Found libretro.py: ${PythonModules_libretro_PATH}")
endif()

if (PythonModules_PIL_PATH)
    message(STATUS "Found PIL (aka Pillow): ${PythonModules_PIL_PATH}")
endif()

if (NOT NDS_ROM)
    message(WARNING "NDS_ROM must be set to the path of an NDS ROM")
    set(NDS_ROM "NDS_ROM-NOTFOUND" CACHE FILEPATH "Path to an NDS ROM" FORCE)
else()
    message(DEBUG "NDS_ROM: ${NDS_ROM}")
endif()

if (NOT GBA_ROM)
    message(WARNING "GBA_ROM must be set to the path of a GBA ROM")
    set(GBA_ROM "GBA_ROM-NOTFOUND" CACHE FILEPATH "Path to a GBA ROM" FORCE)
else()
    message(DEBUG "GBA_ROM: ${GBA_ROM}")
endif()

if (NOT GBA_SRAM)
    message(WARNING "GBA_SRAM must be set to the path of a GBA save file")
    set(GBA_SRAM "GBA_SRAM-NOTFOUND" CACHE FILEPATH "Path to a GBA save file" FORCE)
else()
    message(DEBUG "GBA_SRAM: ${GBA_SRAM}")
endif()

function(find_bios BIOS_NAME BIOS_FILE EXPECTED_BIOS_SIZES KNOWN_BIOS_HASHES)
    if (NOT ${BIOS_NAME})
        find_file(${BIOS_NAME} ${BIOS_FILE} REQUIRED)
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

function(add_python_test)
    set(options
        ARM7_BIOS
        ARM7_DSI_BIOS
        ARM9_BIOS
        ARM9_DSI_BIOS
        DISABLED
        DSI_FIRMWARE
        DSI_NAND
        DSI_SYSFILES
        NDS_FIRMWARE
        NDS_SYSFILES
        NO_SKIP_ERROR_SCREEN
        REQUIRES_OPENGL
        WILL_FAIL
    )

    set(oneValueArgs
        NAME
        SKIP_RETURN_CODE
        SUBSYSTEM
        TEST_MODULE
        TIMEOUT
    )

    set(multiValueArgs
        CONTENT
        CORE_OPTION
        DEPENDS
        FAIL_REGULAR_EXPRESSION
        LABELS
        PASS_REGULAR_EXPRESSION
        SKIP_REGULAR_EXPRESSION
    )
    cmake_parse_arguments(PARSE_ARGV 0 RETRO "${options}" "${oneValueArgs}" "${multiValueArgs}")

    list(LENGTH RETRO_CONTENT RETRO_CONTENT_LENGTH)
    if (RETRO_CONTENT_LENGTH GREATER 0)
        list(GET RETRO_CONTENT 0 NDS_CONTENT)
    endif()

    if (RETRO_CONTENT_LENGTH GREATER 1)
        list(GET RETRO_CONTENT 1 GBA_CONTENT)
    endif()

    if (RETRO_CONTENT_LENGTH GREATER 2)
        list(GET RETRO_CONTENT 2 GBA_SRAM_CONTENT)
    endif()

    add_test(
        NAME "${RETRO_NAME}"
        COMMAND ${Python_EXECUTABLE}
        -m "${RETRO_TEST_MODULE}"
        "$<TARGET_FILE:melondsds_libretro>"
        "${NDS_CONTENT}"
        "${GBA_CONTENT}"
        "${GBA_SRAM_CONTENT}"
    )

    list(APPEND REQUIRED_FILES "$<TARGET_FILE:melondsds_libretro>")
    if (NDS_CONTENT)
        list(APPEND REQUIRED_FILES "${NDS_CONTENT}")
    endif()

    if (GBA_CONTENT)
        list(APPEND REQUIRED_FILES "${GBA_CONTENT}")
    endif()

    if (GBA_SRAM_CONTENT)
        list(APPEND REQUIRED_FILES "${GBA_SRAM_CONTENT}")
    endif()

    if (NOT RETRO_NO_SKIP_ERROR_SCREEN)
        list(APPEND ENVIRONMENT MELONDSDS_SKIP_ERROR_SCREEN=1)
    endif()

    if (RETRO_SUBSYSTEM)
        list(APPEND ENVIRONMENT SUBSYSTEM=${RETRO_SUBSYSTEM})
    endif()

    list(APPEND ENVIRONMENT ${RETRO_CORE_OPTION}) # Not an omission, this is already a list

    macro(expose_system_file SYSFILE)
        if (RETRO_${SYSFILE})
            list(APPEND REQUIRED_FILES "${${SYSFILE}}")
            list(APPEND ENVIRONMENT "${SYSFILE}=${${SYSFILE}}")
        endif()
    endmacro()

    if(${RETRO_NDS_SYSFILES})
        if (NOT RETRO_ARM7_BIOS)
            set(RETRO_ARM7_BIOS 1)
        endif()
        if (NOT RETRO_ARM9_BIOS)
            set(RETRO_ARM9_BIOS 1)
        endif()
        if (NOT RETRO_NDS_FIRMWARE)
            set(RETRO_NDS_FIRMWARE 1)
        endif()
    endif()

    if(${RETRO_DSI_SYSFILES})
        if (NOT RETRO_ARM7_BIOS)
            set(RETRO_ARM7_BIOS 1)
        endif()
        if (NOT RETRO_ARM9_BIOS)
            set(RETRO_ARM9_BIOS 1)
        endif()
        if (NOT RETRO_ARM7_DSI_BIOS)
            set(RETRO_ARM7_DSI_BIOS 1)
        endif()
        if (NOT RETRO_ARM9_DSI_BIOS)
            set(RETRO_ARM9_DSI_BIOS 1)
        endif()
        if (NOT RETRO_DSI_FIRMWARE)
            set(RETRO_DSI_FIRMWARE 1)
        endif()
        if (NOT RETRO_DSI_NAND)
            set(RETRO_DSI_NAND 1)
        endif()
    endif()

    expose_system_file(ARM7_BIOS)
    expose_system_file(ARM9_BIOS)
    expose_system_file(ARM7_DSI_BIOS)
    expose_system_file(ARM9_DSI_BIOS)
    expose_system_file(NDS_FIRMWARE)
    expose_system_file(DSI_FIRMWARE)
    expose_system_file(DSI_NAND)

    set_tests_properties("${RETRO_NAME}" PROPERTIES LABELS "libretro.py;${RETRO_LABELS}")
    set_tests_properties("${RETRO_NAME}" PROPERTIES ENVIRONMENT "${ENVIRONMENT}")
    set_tests_properties("${RETRO_NAME}" PROPERTIES REQUIRED_FILES "${REQUIRED_FILES}")
    set_tests_properties("${RETRO_NAME}" PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/python")
    if (RETRO_PASS_REGULAR_EXPRESSION)
        set_tests_properties("${RETRO_NAME}" PROPERTIES PASS_REGULAR_EXPRESSION "${RETRO_PASS_REGULAR_EXPRESSION}")
    endif()

    set_tests_properties("${RETRO_NAME}" PROPERTIES FAIL_REGULAR_EXPRESSION "Exception ignored on calling ctypes callback function")
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
    elseif(RETRO_REQUIRES_OPENGL AND NOT HAVE_OPENGL)
        message(WARNING "Test '${RETRO_NAME}' requires OpenGL, but the core wasn't built with it. Disabling.")
        set_tests_properties("${RETRO_NAME}" PROPERTIES DISABLED TRUE)
    endif()

    if (RETRO_SKIP_RETURN_CODE)
        set_tests_properties("${RETRO_NAME}" PROPERTIES SKIP_RETURN_CODE "${RETRO_SKIP_RETURN_CODE}")
    endif()

    if (RETRO_DEPENDS)
        set_tests_properties("${RETRO_NAME}" PROPERTIES DEPENDS "${RETRO_DEPENDS}")
    endif()

    if (RETRO_TIMEOUT)
        set_tests_properties("${RETRO_NAME}" PROPERTIES TIMEOUT "${RETRO_TIMEOUT}")
    else()
        set_tests_properties("${RETRO_NAME}" PROPERTIES TIMEOUT 10)
    endif()
endfunction()

FetchContent_Declare(
    godmode9i
    URL "https://github.com/DS-Homebrew/GodMode9i/releases/download/v3.4.1/GodMode9i.7z"
)

FetchContent_GetProperties(godmode9i)
FetchContent_MakeAvailable(godmode9i)
set(GODMODE9I_ROM "${godmode9i_SOURCE_DIR}/GodMode9i.nds")

set(MICRECORD_NDS "${CMAKE_CURRENT_SOURCE_DIR}/nds/micrecord.nds")

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

include(cmake/Basics.cmake)
include(cmake/Booting.cmake)
include(cmake/Errors.cmake)
include(cmake/Firmware.cmake)
include(cmake/Reset.cmake)
include(cmake/TestHomebrewSDCards.cmake)
include(cmake/Video.cmake)