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

    message(STATUS "Using ${name}: ${${VAR_NAME}_REPOSITORY_URL} (ref ${${VAR_NAME}_REPOSITORY_TAG})")

    FetchContent_Declare(
        ${name}
        GIT_REPOSITORY "${${VAR_NAME}_REPOSITORY_URL}"
        GIT_TAG "${${VAR_NAME}_REPOSITORY_TAG}"
    )

    FetchContent_GetProperties(${name})
endfunction()

fetch_dependency(melonDS "https://github.com/melonDS-emu/melonDS" "d48e5f2")
fetch_dependency("libretro-common" "https://github.com/libretro/libretro-common.git" "fce57fd")
fetch_dependency("embed-binaries" "https://github.com/andoalon/embed-binaries.git" "21f28ca")
fetch_dependency(glm "https://github.com/g-truc/glm" "33b0eb9")
fetch_dependency(libslirp "https://github.com/JesseTG/libslirp-mirror.git" "44e7877")
fetch_dependency(pntr "https://github.com/robloach/pntr" "3421a6d")
fetch_dependency(fmt "https://github.com/fmtlib/fmt" "10.2.1")
fetch_dependency(yamc "https://github.com/yohhoy/yamc" "4e015a7")
fetch_dependency(span-lite "https://github.com/martinmoene/span-lite" "bc08bf8")
fetch_dependency(date "https://github.com/HowardHinnant/date" "88a3b15")

# We build zlib from source because some distributions (e.g. Ubuntu) ship a static library
# that wasn't compiled with -fPIC, which causes linking errors when building a shared library.
fetch_dependency(zlib "https://github.com/madler/zlib.git" "v1.3")

if (TRACY_ENABLE)
    fetch_dependency(tracy "https://github.com/wolfpld/tracy.git" "v0.10")
endif()

set(CMAKE_MODULE_PATH "${FETCHCONTENT_BASE_DIR}/melonds-src/cmake" "${FETCHCONTENT_BASE_DIR}/embed-binaries-src/cmake" "${CMAKE_MODULE_PATH}")
set(BUILD_STATIC ON)
set(BUILD_STATIC_LIBS ON)
set(BUILD_QT_SDL OFF)
set(ENABLE_GDBSTUB OFF)
set(GLM_BUILD_LIBRARY ON CACHE BOOL "" FORCE)
set(GLM_ENABLE_CXX_17 ON CACHE BOOL "" FORCE)
option(ENABLE_TESTING "Enable unit testing." OFF)
FetchContent_MakeAvailable(melonDS libretro-common embed-binaries glm zlib libslirp pntr fmt yamc span-lite date)

if (TRACY_ENABLE)
    set(BUILD_SHARED_LIBS OFF)
    option(TRACY_DELAYED_INIT "" ON)
    option(TRACY_MANUAL_LIFETIME "" ON)
    option(TRACY_STATIC "" ON)
    FetchContent_MakeAvailable(tracy)
endif()

set_target_properties(example minigzip PROPERTIES EXCLUDE_FROM_ALL TRUE)
if(HAVE_OFF64_T)
    set_target_properties(example64 minigzip64 PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()