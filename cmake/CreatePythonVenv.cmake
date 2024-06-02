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
    if (NOT EXISTS ${VENV_PATH})
        FIND_PACKAGE(Python3 COMPONENTS Interpreter)
        MESSAGE(STATUS "Creating Python virtual environment at ${VENV_PATH} with ${Python3_EXECUTABLE}")
        EXECUTE_PROCESS(COMMAND ${Python3_EXECUTABLE} "-m" "venv" ${venv_name}
                WORKING_DIRECTORY ${venv_dest}
                ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE)
    endif()

    # check if virtual environment was made successfully/already exists (path exists)
    if (EXISTS ${VENV_PATH})
        # unset python executable
        UNSET(Python3_EXECUTABLE)

        # make cmake find the python executable in virtual environment instead
        SET(ENV{VIRTUAL_ENV} ${VENV_PATH})
        SET(Python3_FIND_VIRTUALENV FIRST)
        SET(Python3_FIND_REGISTRY NEVER)
        SET(CMAKE_FIND_FRAMEWORK NEVER)
        FIND_PACKAGE(Python3 COMPONENTS Interpreter Development)

        # make a regex that can be used to check whether a path is within the venv
        # need to replace escape all special regex characters in the path
        string(REGEX REPLACE "([][+.*()^])" "\\\\\\1" VENV_PATH_AS_REGEX "${VENV_PATH}")
        SET(IN_VENV_REGEX "${VENV_PATH_AS_REGEX}/+")

        # If python executable found in the venv path
        if (Python3_FOUND AND Python3_EXECUTABLE MATCHES "${IN_VENV_REGEX}")
            # set return variable in parent scope
            SET(venv_executable "${Python3_EXECUTABLE}")
            SET(${out_venv_executable} "${venv_executable}" PARENT_SCOPE)

            # if given path to requirements.txt try install/update them
            if (DEFINED ARGV3)
                MESSAGE(STATUS "Checking/installing python requirements")
                EXECUTE_PROCESS(COMMAND ${venv_executable} -m pip install --upgrade pip setuptools -q)
                EXECUTE_PROCESS(COMMAND ${venv_executable} -m pip install --upgrade -r "${ARGV3}" -q)
                MESSAGE(STATUS "Python requirements updated")
            endif()
        else()
            MESSAGE(WARNING "Python virtual environment creation failed.")
        endif()
    endif()
endfunction()