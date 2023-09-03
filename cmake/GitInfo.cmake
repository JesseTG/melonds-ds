# Copyright (c) 2011-2014 Stefan Eilemann <eile@eyescale.ch>

# Sets the following variables if git is found:
# GIT_REVISION: The current HEAD sha hash
# GIT_STATE: A description of the working tree, e.g., 1.8.0-48-g6d23f80-dirty
# GIT_ORIGIN_URL: The origin of the working tree
# GIT_ROOT_URL: The root remote of the working tree
# GIT_BRANCH: The name of the current branch
# GIT_AUTHORS: A list of all authors in the git history

if(GIT_INFO_DONE_${PROJECT_NAME})
    return()
endif()

set(GIT_INFO_DONE_${PROJECT_NAME} ON)
set(GIT_STATE)
set(GIT_ORIGIN_URL)
set(GIT_ROOT_URL)
set(GIT_BRANCH)

if(EXISTS ${PROJECT_SOURCE_DIR}/.git)
    if(NOT GIT_FOUND)
        find_package(Git QUIET)
    endif()
    if(GIT_FOUND)
        execute_process( COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process( COMMAND "${GIT_EXECUTABLE}" describe --long --tags --dirty --always
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_STATE OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        execute_process( COMMAND "${GIT_EXECUTABLE}" config --get remote.origin.url
                OUTPUT_VARIABLE GIT_ORIGIN_URL OUTPUT_STRIP_TRAILING_WHITESPACE
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
        execute_process( COMMAND "${GIT_EXECUTABLE}" config --get remote.root.url
                OUTPUT_VARIABLE GIT_ROOT_URL OUTPUT_STRIP_TRAILING_WHITESPACE
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
        execute_process( COMMAND "${GIT_EXECUTABLE}" branch --contains HEAD
                OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

        if(EXISTS ${PROJECT_BINARY_DIR}/Authors.txt)
            file(READ ${PROJECT_BINARY_DIR}/Authors.txt GIT_AUTHORS)
        else()
            # cache authors to not slow down cmake runs
            set(GIT_AUTHORS)
            execute_process(COMMAND "${GIT_EXECUTABLE}" log
                    OUTPUT_VARIABLE GIT_LOG OUTPUT_STRIP_TRAILING_WHITESPACE
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
            string(REPLACE "\n" ";" GIT_LOG ${GIT_LOG})
            foreach(__line ${GIT_LOG})
                if(__line MATCHES "^Author:")
                    string(REPLACE "Author: " "" __line "${__line}")
                    string(REGEX REPLACE "[ ]?<.*" "" __line "${__line}")
                    list(APPEND GIT_AUTHORS "${__line}")
                endif()
            endforeach()
            list(SORT GIT_AUTHORS)
            list(REMOVE_DUPLICATES GIT_AUTHORS)
            file(WRITE ${PROJECT_BINARY_DIR}/Authors.txt "${GIT_AUTHORS}")
        endif()

        if(NOT GIT_ROOT_URL)
            set(GIT_ROOT_URL ${GIT_ORIGIN_URL})
        endif()
        if(NOT GIT_STATE)
            set(GIT_STATE "<no-tag>")
        endif()
        if(GIT_BRANCH)
            string(REPLACE "* " "" GIT_BRANCH ${GIT_BRANCH})
        endif()

    else()
        message(STATUS "No revision version support, git not found")
    endif()
endif()

if(NOT GIT_REVISION)
    set(GIT_REVISION "0")
endif()