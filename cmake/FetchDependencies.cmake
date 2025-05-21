macro(define_git_dependency_vars name default_url default_tag)
    string(TOUPPER ${name} VAR_NAME)
    string(MAKE_C_IDENTIFIER ${VAR_NAME} VAR_NAME)

    if (NOT ${VAR_NAME}_REPOSITORY_URL)
        set(
                "${VAR_NAME}_REPOSITORY_URL"
                "${default_url}"
                CACHE STRING
                "${name} repository URL. Set this to use a fork."
                FORCE
        )
    endif ()

    if (NOT ${VAR_NAME}_REPOSITORY_TAG)
        set(
                "${VAR_NAME}_REPOSITORY_TAG"
                "${default_tag}"
                CACHE STRING
                "${name} repository commit hash or tag. Set this when using a new version or a custom branch."
                FORCE
        )
    endif ()
endmacro()

function(fetch_dependency name default_url default_tag)
    define_git_dependency_vars(${name} ${default_url} ${default_tag})

    if (FETCHCONTENT_SOURCE_DIR_${VAR_NAME})
        message(STATUS "Using ${name}: ${FETCHCONTENT_SOURCE_DIR_${VAR_NAME}} (local)")
    else()
        message(STATUS "Using ${name}: ${${VAR_NAME}_REPOSITORY_URL} (ref ${${VAR_NAME}_REPOSITORY_TAG})")
    endif()

    FetchContent_Declare(
        ${name}
        GIT_REPOSITORY "${${VAR_NAME}_REPOSITORY_URL}"
        GIT_TAG "${${VAR_NAME}_REPOSITORY_TAG}"
    )

    FetchContent_GetProperties(${name})
endfunction()

fetch_dependency(melonDS "https://github.com/melonDS-emu/melonDS" "7baeb26")
fetch_dependency(libretro-common "https://github.com/JesseTG/libretro-common" "8e2b884")
fetch_dependency("embed-binaries" "https://github.com/andoalon/embed-binaries.git" "21f28ca")
fetch_dependency(glm "https://github.com/g-truc/glm" "33b4a62")
fetch_dependency(libslirp "https://github.com/JesseTG/libslirp-mirror" "e61dbd4")
fetch_dependency(pntr "https://github.com/robloach/pntr" "650237a")
fetch_dependency(fmt "https://github.com/fmtlib/fmt" "11.0.2")
fetch_dependency(yamc "https://github.com/yohhoy/yamc" "4e015a7")
fetch_dependency(span-lite "https://github.com/martinmoene/span-lite" "00afc28")
fetch_dependency(date "https://github.com/HowardHinnant/date" "1ead671")

# We build zlib from source because some distributions (e.g. Ubuntu) ship a static library
# that wasn't compiled with -fPIC, which causes linking errors when building a shared library.
fetch_dependency(zlib "https://github.com/madler/zlib" "v1.3.1")

if (TRACY_ENABLE)
    fetch_dependency(tracy "https://github.com/wolfpld/tracy" "v0.11.1")
endif()

set(CMAKE_MODULE_PATH "${FETCHCONTENT_BASE_DIR}/melonds-src/cmake" "${FETCHCONTENT_BASE_DIR}/embed-binaries-src/cmake" "${CMAKE_MODULE_PATH}")
set(BUILD_STATIC ON)
set(BUILD_STATIC_LIBS ON)
set(BUILD_QT_SDL OFF)
set(ENABLE_GDBSTUB OFF)
set(GLM_BUILD_LIBRARY ON CACHE BOOL "" FORCE)
set(GLM_ENABLE_CXX_17 ON CACHE BOOL "" FORCE)
option(ENABLE_TESTING "Enable unit testing." OFF)
if (${CMAKE_MAJOR_VERSION} VERSION_GREATER_EQUAL 4)
    # Needed for https://github.com/JesseTG/melonds-ds/issues/265
    # yamc's stated minimum CMake version is 3.2,
    # but it configures and builds fine with CMake 4.
    # Setting the policy version to 3.5
    # lets us fix this without having to fork yamc.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif ()
FetchContent_MakeAvailable(melonDS libretro-common embed-binaries glm zlib libslirp pntr fmt yamc span-lite date)

if (TRACY_ENABLE)
    set(BUILD_SHARED_LIBS OFF)
    option(TRACY_DELAYED_INIT "" ON)
    option(TRACY_MANUAL_LIFETIME "" ON)
    option(TRACY_ON_DEMAND "" ON)
    option(TRACY_STATIC "" ON)
    FetchContent_MakeAvailable(tracy)
endif()

set_target_properties(example minigzip PROPERTIES EXCLUDE_FROM_ALL TRUE)
if(HAVE_OFF64_T)
    set_target_properties(example64 minigzip64 PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()