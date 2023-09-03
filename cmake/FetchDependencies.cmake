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

fetch_dependency("melonDS" "https://github.com/melonDS-emu/melonDS.git" "0947e94")
fetch_dependency("libretro-common" "https://github.com/libretro/libretro-common.git" "01c6122")
fetch_dependency("embed-binaries" "https://github.com/andoalon/embed-binaries.git" "21f28ca")
fetch_dependency(glm "https://github.com/g-truc/glm.git" "5c46b9c")
fetch_dependency(libslirp "https://github.com/JesseTG/libslirp-mirror.git" "44e7877")

# We build zlib from source because some distributions (e.g. Ubuntu) ship a static library
# that wasn't compiled with -fPIC, which causes linking errors when building a shared library.
fetch_dependency(zlib "https://github.com/madler/zlib.git" "v1.2.13")

if (TRACY_ENABLE)
    fetch_dependency(tracy "https://github.com/wolfpld/tracy.git" "v0.9.1")
endif()

set(CMAKE_MODULE_PATH "${FETCHCONTENT_BASE_DIR}/melonds-src/cmake" "${FETCHCONTENT_BASE_DIR}/embed-binaries-src/cmake" "${CMAKE_MODULE_PATH}")
set(BUILD_STATIC ON)
set(BUILD_STATIC_LIBS ON)
set(BUILD_QT_SDL OFF)
FetchContent_MakeAvailable(melonDS libretro-common embed-binaries glm zlib libslirp)

if (TRACY_ENABLE)
    FetchContent_MakeAvailable(tracy)
    set(BUILD_SHARED_LIBS OFF)
    set(TRACY_STATIC ON)
    set(TRACY_ON_DEMAND ON)
endif()
