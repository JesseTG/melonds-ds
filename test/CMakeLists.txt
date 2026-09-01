option(MELONDSDS_INTERNAL_VENV "Use an internal Python virtual environment for the test suite; disable if using your own." ON)
option(MELONDSDS_INSTALL_TEST_REQUIREMENTS "Install test/requirements.txt into the Python environment used by the test suite." ON)

# libretro.py 0.8.x requires Python 3.12 or later.
find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)

if (MELONDSDS_INTERNAL_VENV)
    include(../cmake/CreatePythonVenv.cmake)
    if (MELONDSDS_INSTALL_TEST_REQUIREMENTS)
        CreatePythonVenv("${CMAKE_BINARY_DIR}" "venv" VENV "${CMAKE_CURRENT_LIST_DIR}/requirements.txt")
    else()
        CreatePythonVenv("${CMAKE_BINARY_DIR}" "venv" VENV)
    endif()
    set(PYTHON_EXECUTABLE "${VENV}")
else()
    # FindPython sets "Python3_EXECUTABLE", but the rest of this file wants "PYTHON_EXECUTABLE"
    set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")

    if (MELONDSDS_INSTALL_TEST_REQUIREMENTS)
        message(STATUS "Installing test requirements into ${PYTHON_EXECUTABLE}")
        execute_process(
            COMMAND "${PYTHON_EXECUTABLE}" -m pip install --upgrade -r "${CMAKE_CURRENT_LIST_DIR}/requirements.txt" -q
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()
endif()

message(STATUS "Using Python environment at ${PYTHON_EXECUTABLE}")

################################################################################
# Locating pytest and pytest-cmake
################################################################################

# Everything below must resolve against ${PYTHON_EXECUTABLE}, not against whatever
# happens to be first on PATH.
execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" -c "import sysconfig; print(sysconfig.get_path('scripts'))"
    OUTPUT_VARIABLE PYTHON_SCRIPTS_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)
file(TO_CMAKE_PATH "${PYTHON_SCRIPTS_DIR}" PYTHON_SCRIPTS_DIR)

set(REQUIREMENTS_HINT
    "Run \"${PYTHON_EXECUTABLE}\" -m pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt, or configure with -DMELONDSDS_INSTALL_TEST_REQUIREMENTS=ON.")

# FindPytest.cmake does a bare find_program(PYTEST_EXECUTABLE NAMES pytest), which is
# a no-op if the cache entry is already set. Seeding it here is how we pin pytest to
# the interpreter we chose above.
find_program(PYTEST_EXECUTABLE
    NAMES pytest
    HINTS "${PYTHON_SCRIPTS_DIR}"
    NO_DEFAULT_PATH
    DOC "The pytest executable belonging to PYTHON_EXECUTABLE"
)

if (NOT PYTEST_EXECUTABLE)
    message(FATAL_ERROR "pytest not found for ${PYTHON_EXECUTABLE}. ${REQUIREMENTS_HINT}")
endif()

execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" -m pytest_cmake --cmake-dir
    OUTPUT_VARIABLE Pytest_ROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE PYTEST_CMAKE_RESULT
)

if (NOT PYTEST_CMAKE_RESULT EQUAL 0)
    message(FATAL_ERROR "pytest-cmake is not installed for ${PYTHON_EXECUTABLE}. ${REQUIREMENTS_HINT}")
endif()

file(TO_CMAKE_PATH "${Pytest_ROOT}" Pytest_ROOT)
find_package(Pytest 8.0 REQUIRED)
message(STATUS "Found pytest ${PYTEST_VERSION}: ${PYTEST_EXECUTABLE}")

# Report the libretro.py version so a mismatch is obvious in the configure log.
execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" -c "import libretro, importlib.metadata as m; print(m.version('libretro.py'))"
    OUTPUT_VARIABLE LIBRETRO_PY_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE LIBRETRO_PY_RESULT
)

if (NOT LIBRETRO_PY_RESULT EQUAL 0)
    message(FATAL_ERROR "libretro.py is not installed for ${PYTHON_EXECUTABLE}. ${REQUIREMENTS_HINT}")
endif()

message(STATUS "Found libretro.py ${LIBRETRO_PY_VERSION}")

################################################################################
# Test content
################################################################################
#
# Assets are optional. A missing one leaves a <VAR>-NOTFOUND value in the cache,
# which conftest.py turns into a runtime skip for the tests that need it.

function(find_content VAR DESCRIPTION)
    if (NOT ${VAR})
        message(WARNING "${VAR} is unset; tests that need ${DESCRIPTION} will be skipped")
        set(${VAR} "${VAR}-NOTFOUND" CACHE FILEPATH "Path to ${DESCRIPTION}" FORCE)
    else()
        message(DEBUG "${VAR}: ${${VAR}}")
    endif()
endfunction()

find_content(NDS_ROM "an NDS ROM")
find_content(DSIWARE_ROM "a DSiWare ROM (not a DSi-enhanced cartridge)")
find_content(GBA_ROM "a GBA ROM")
find_content(GBA_SRAM "a GBA save file")

FetchContent_Declare(
    godmode9i
    URL "https://github.com/DS-Homebrew/GodMode9i/releases/download/v3.4.1/GodMode9i.7z"
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

FetchContent_GetProperties(godmode9i)
FetchContent_MakeAvailable(godmode9i)
set(GODMODE9I_ROM "${godmode9i_SOURCE_DIR}/GodMode9i.nds")
set(GODMODE9I_DSI_ROM "${godmode9i_SOURCE_DIR}/GodMode9i.dsi")

set(MICRECORD_NDS "${CMAKE_CURRENT_SOURCE_DIR}/nds/micrecord.nds")
set(PERIPH_SLOT2_NDS "${CMAKE_CURRENT_SOURCE_DIR}/nds/periph_slot2.nds")
set(SCFG_REGISTERS_NDS "${CMAKE_CURRENT_SOURCE_DIR}/nds/tests_sys_scfg_registers.nds")

################################################################################
# System files
################################################################################
#
# Also optional, but a file that *is* present has to be the real thing: a wrong
# size or an unrecognized hash is a configuration error, not a skip.

function(find_system_file VAR FILENAME EXPECTED_SIZES KNOWN_HASHES)
    if (NOT ${VAR})
        find_file(${VAR} ${FILENAME})
    endif()

    if (NOT ${VAR})
        message(WARNING "${VAR} (${FILENAME}) not found; tests that need it will be skipped")
        set(${VAR} "${VAR}-NOTFOUND" CACHE FILEPATH "Path to ${FILENAME}" FORCE)
        return()
    endif()

    file(SIZE "${${VAR}}" BIOS_SIZE)
    if (NOT (BIOS_SIZE IN_LIST EXPECTED_SIZES))
        message(SEND_ERROR "Expected ${VAR} with size in ${EXPECTED_SIZES}, got ${BIOS_SIZE}")
    endif()

    file(MD5 "${${VAR}}" BIOS_MD5)
    string(TOLOWER "${BIOS_MD5}" BIOS_MD5)
    if (KNOWN_HASHES AND NOT (BIOS_MD5 IN_LIST KNOWN_HASHES))
        message(SEND_ERROR "Expected ${VAR} with an MD5 in ${KNOWN_HASHES}, got \"${BIOS_MD5}\"")
    endif()

    message(DEBUG "${VAR}: ${${VAR}} (${BIOS_SIZE} bytes, MD5=${BIOS_MD5})")
endfunction()

set(ARM7_BIOS_HASHES "df692a80a5b1bc90728bc3dfc76cd948") # NDS ARM7 BIOS
find_system_file(ARM7_BIOS bios7.bin 16384 "${ARM7_BIOS_HASHES}")

set(ARM9_BIOS_HASHES
    "a392174eb3e572fed6447e956bde4b25" # NDS ARM9 BIOS (World)
    "08f1dc42ebe85f129b06415f5e2ccbc8" # NDS ARM9 BIOS (China)
)
find_system_file(ARM9_BIOS bios9.bin 4096 "${ARM9_BIOS_HASHES}")

# These images legitimately vary between revisions, so they're size-checked only.
find_system_file(ARM7_DSI_BIOS dsi_bios7.bin 65536 "")
find_system_file(ARM9_DSI_BIOS dsi_bios9.bin 65536 "")
find_system_file(NDS_FIRMWARE firmware.bin "131072;262144;524288" "")
find_system_file(DSI_FIRMWARE dsi_firmware.bin "131072;262144;524288" "")
find_system_file(DSI_NAND dsi_nand.bin 251658304 "")

################################################################################
# Library search path
################################################################################

if (NOT DEFINED TEST_EXCLUDE_PATHS)
    set(TEST_EXCLUDE_PATHS "" CACHE PATH "List of paths to exclude from the library search path for each test. Use this to work around IDEs that set the PATH to something with an incompatible opengl32.dll, like CLion.")
endif ()

function(filter_path original_path exclude_paths result_var)
    cmake_path(CONVERT "${original_path}" TO_CMAKE_PATH_LIST entries NORMALIZE)

    set(kept "")
    foreach (entry IN LISTS entries)
        set(excluded FALSE)
        foreach (exclude IN LISTS exclude_paths)
            cmake_path(COMPARE "${entry}" EQUAL "${exclude}" same)
            if (same)
                set(excluded TRUE)
            endif()
        endforeach()

        if (NOT excluded)
            list(APPEND kept "${entry}")
        endif()
    endforeach()

    cmake_path(CONVERT "${kept}" TO_NATIVE_PATH_LIST native)
    set(${result_var} "${native}" PARENT_SCOPE)
endfunction()

if (WIN32)
    set(LIBRARY_ENV_NAME PATH)
elseif (APPLE)
    set(LIBRARY_ENV_NAME DYLD_LIBRARY_PATH)
else()
    set(LIBRARY_ENV_NAME LD_LIBRARY_PATH)
endif()

# pytest_discover_tests() snapshots this environment variable at call time and
# re-emits it into every generated test's ENVIRONMENT property, correctly escaped.
# Filtering the real variable around the call is therefore both simpler and more
# robust than trying to out-escape pytest-cmake's own escaping.
set(SAVED_LIBRARY_PATH "$ENV{${LIBRARY_ENV_NAME}}")
filter_path("${SAVED_LIBRARY_PATH}" "${TEST_EXCLUDE_PATHS}" FILTERED_LIBRARY_PATH)
set(ENV{${LIBRARY_ENV_NAME}} "${FILTERED_LIBRARY_PATH}")

################################################################################
# Test discovery
################################################################################

set(DEFAULT_TIMEOUT 60) # In seconds

# Re-run discovery whenever a test module or conftest.py changes.
file(GLOB MELONDSDS_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/python/*.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/python/melondsds/*.py"
)

# pytest.toml has to be listed by name rather than matched by the glob above:
# CONFIGURE_DEPENDS only notices when the set of matching files changes,
# and this one always exists.
# It belongs in DEPENDS because it declares every marker the suite uses
# and sets the rootdir, so editing it can change what gets collected at all.
list(APPEND MELONDSDS_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/python/pytest.toml")

# One CTest test per collected pytest node, each in its own process.
# ENVIRONMENT and PROPERTIES apply uniformly to all of them; anything that used to
# vary per test (core options, staged system files, WILL_FAIL, DISABLED, per-test
# timeouts) now lives in the test modules as markers and parametrization.
pytest_discover_tests(melonDSDS
    TEST_PATHS "."
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/python"

    # Produces names like "melonDSDS.test_booting.boots.direct-nds-builtin".
    # STRIP_PARAM_BRACKETS matters because CTest's -R and -E take regexes, and
    # square brackets are metacharacters there.
    INCLUDE_FILE_PATH
    STRIP_PARAM_BRACKETS
    TRIM_FROM_NAME "^test_"

    LIBRARY_PATH_PREPEND "$<TARGET_FILE_DIR:melondsds_libretro>"
    DEPENDS melondsds_libretro ${MELONDSDS_TEST_SOURCES}

    ENVIRONMENT
        "MELONDSDS_CORE=$<TARGET_FILE:melondsds_libretro>"
        "MELONDSDS_SKIP_ERROR_SCREEN=1"
        "MELONDSDS_HAVE_OPENGL=$<BOOL:${HAVE_OPENGL}>"
        "NDS_ROM=${NDS_ROM}"
        "DSIWARE_ROM=${DSIWARE_ROM}"
        "GBA_ROM=${GBA_ROM}"
        "GBA_SRAM=${GBA_SRAM}"
        "GODMODE9I_ROM=${GODMODE9I_ROM}"
        "GODMODE9I_DSI_ROM=${GODMODE9I_DSI_ROM}"
        "MICRECORD_NDS=${MICRECORD_NDS}"
        "PERIPH_SLOT2_NDS=${PERIPH_SLOT2_NDS}"
        "SCFG_REGISTERS_NDS=${SCFG_REGISTERS_NDS}"
        "ARM7_BIOS=${ARM7_BIOS}"
        "ARM9_BIOS=${ARM9_BIOS}"
        "ARM7_DSI_BIOS=${ARM7_DSI_BIOS}"
        "ARM9_DSI_BIOS=${ARM9_DSI_BIOS}"
        "NDS_FIRMWARE=${NDS_FIRMWARE}"
        "DSI_FIRMWARE=${DSI_FIRMWARE}"
        "DSI_NAND=${DSI_NAND}"
        "PYTHONUNBUFFERED=1"
        "PYTHONDONTWRITEBYTECODE=1"

    PROPERTIES
        # Keep this a single label: PROPERTIES is a multi-value argument, so a
        # semicolon would split it into two property tokens.
        LABELS libretro.py
        TIMEOUT ${DEFAULT_TIMEOUT}
        # conftest.py exits with 77 when a test case was skipped for want of an asset.
        SKIP_RETURN_CODE 77
        # CPython prints this when an exception escapes a ctypes callback. pytest.toml
        # also promotes the corresponding warning to an error; this catches whatever
        # slips past that.
        FAIL_REGULAR_EXPRESSION "Exception ignored on calling ctypes callback function"

    EXTRA_ARGS -p no:cacheprovider --no-header
    DISCOVERY_EXTRA_ARGS -p no:cacheprovider
)

set(ENV{${LIBRARY_ENV_NAME}} "${SAVED_LIBRARY_PATH}")

# TODO: Write the following tests:

# - Core loads BIOS from system
# - Core loads BIOS from system/melonDS DS
# - Core loads NDS game with GBA game in slot-2
# - Core loads NDS game with GBA game in slot-2 and save data
# - Core loads NDS game with GBA game in slot-2 and save data, and can save GBA data
# (will need to find a test case besides Pokemon)
