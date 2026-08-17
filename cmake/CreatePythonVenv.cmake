function(CreatePythonVenv venv_dest venv_name out_venv_executable)
    # Creates a virtual environment within the binary directory, with the given input name. Returns the path to the
    # virtual environments python executable for use else where. Optional argument of path to a requirements.txt, if
    # given will attempt to install/update within virtual environment.
    ## ARGUMENTS
    # venv_dest            - Input the destination path for the virtual environment to be created within
    # venv_name            - Input name of the virtual environment to create in the binary directory
    # out_venv_executable  - Variable to be set in the parent scope, holds path to virtual environment executable
    ## OPTIONAL
    # requirements_path    - Optional input of path to requirements.txt

    # create variable for venv path from input
    SET(VENV_PATH "${venv_dest}/${venv_name}")

    # if path doesn't exist, create the virtual environment
    if (NOT EXISTS "${VENV_PATH}")
        FIND_PACKAGE(Python3 COMPONENTS Interpreter REQUIRED)
        MESSAGE(STATUS "Creating Python virtual environment at ${VENV_PATH} with ${Python3_EXECUTABLE}")
        EXECUTE_PROCESS(COMMAND ${Python3_EXECUTABLE} "-m" "venv" ${venv_name}
                WORKING_DIRECTORY ${venv_dest}
                ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE
                COMMAND_ERROR_IS_FATAL ANY)
    endif()

    # Derive the venv's interpreter from its well-known layout instead of calling
    # FIND_PACKAGE(Python3) again. The hints that pick a specific base interpreter
    # (Python3_ROOT_DIR, Python3_EXECUTABLE) outrank Python3_FIND_VIRTUALENV, so a
    # second FIND_PACKAGE would silently resolve back to the base installation.
    if (CMAKE_HOST_WIN32)
        SET(venv_executable "${VENV_PATH}/Scripts/python.exe")
    else()
        SET(venv_executable "${VENV_PATH}/bin/python")
    endif()

    if (NOT EXISTS "${venv_executable}")
        MESSAGE(FATAL_ERROR "Python virtual environment creation failed; no interpreter at ${venv_executable}")
    endif()

    # set return variable in parent scope
    SET(${out_venv_executable} "${venv_executable}" PARENT_SCOPE)

    # if given path to requirements.txt try install/update them
    if (DEFINED ARGV3)
        MESSAGE(STATUS "Checking/installing python requirements")
        EXECUTE_PROCESS(COMMAND "${venv_executable}" -m pip install --upgrade pip setuptools -q
                COMMAND_ERROR_IS_FATAL ANY)
        EXECUTE_PROCESS(COMMAND "${venv_executable}" -m pip install --upgrade -r "${ARGV3}" -q
                COMMAND_ERROR_IS_FATAL ANY)
        MESSAGE(STATUS "Python requirements updated")
    endif()
endfunction()