if (NOT MELONDS_REPOSITORY_URL)
    set(
        MELONDS_REPOSITORY_URL
        "https://github.com/melonDS-emu/melonDS.git"
        CACHE STRING
        "melonDS repository URL. Set this to use a melonDS fork or mirror."
        FORCE
    )
endif ()

if (NOT MELONDS_REPOSITORY_TAG)
    set(
        MELONDS_REPOSITORY_TAG
        "0947e94"
        CACHE STRING
        "melonDS repository commit hash or tag. Set this when using a new version of melonDS, or when using a custom branch."
        FORCE
    )
endif ()

message(STATUS "Using melonDS: ${MELONDS_REPOSITORY_URL} (ref ${MELONDS_REPOSITORY_TAG})")

FetchContent_Declare(
    melonDS
    GIT_REPOSITORY "${MELONDS_REPOSITORY_URL}"
    GIT_TAG "${MELONDS_REPOSITORY_TAG}"
)

FetchContent_GetProperties(melonDS)

if (NOT LIBRETRO_COMMON_REPOSITORY_URL)
    set(
        LIBRETRO_COMMON_REPOSITORY_URL
        "https://github.com/libretro/libretro-common.git"
        CACHE STRING
        "libretro-common repository URL. Set this to use a fork or mirror."
        FORCE
    )
endif ()

if (NOT LIBRETRO_COMMON_REPOSITORY_TAG)
    set(
        LIBRETRO_COMMON_REPOSITORY_TAG
        "10995d5"
        CACHE STRING
        "libretro-common repository commit hash or tag. Set this when using a new version or a custom branch."
        FORCE
    )
endif ()

message(STATUS "Using libretro-common: ${LIBRETRO_COMMON_REPOSITORY_URL} (ref ${LIBRETRO_COMMON_REPOSITORY_TAG})")

FetchContent_Declare(
    libretro-common
    GIT_REPOSITORY "${LIBRETRO_COMMON_REPOSITORY_URL}"
    GIT_TAG "${LIBRETRO_COMMON_REPOSITORY_TAG}"
)
FetchContent_GetProperties(libretro-common)

if (NOT EMBED_BINARIES_REPOSITORY_URL)
    set(
        EMBED_BINARIES_REPOSITORY_URL
        "https://github.com/andoalon/embed-binaries.git"
        CACHE STRING
        "embed-binaries repository URL. Set this to use an embed-binaries fork or mirror."
        FORCE
    )
endif ()

if (NOT EMBED_BINARIES_REPOSITORY_TAG)
    set(
        EMBED_BINARIES_REPOSITORY_TAG
        "21f28ca"
        CACHE STRING
        "embed-binaries repository commit hash or tag. Set this when using a new version of embed-binaries, or when using a custom branch."
        FORCE
    )
endif ()

message(STATUS "Using embed-binaries: ${EMBED_BINARIES_REPOSITORY_URL} (ref ${EMBED_BINARIES_REPOSITORY_TAG})")

FetchContent_Declare(
    embed-binaries
    GIT_REPOSITORY "${EMBED_BINARIES_REPOSITORY_URL}"
    GIT_TAG "${EMBED_BINARIES_REPOSITORY_TAG}"
)
FetchContent_GetProperties(embed-binaries)

set(CMAKE_MODULE_PATH "${FETCHCONTENT_BASE_DIR}/melonds-src/cmake" "${FETCHCONTENT_BASE_DIR}/embed-binaries-src/cmake" "${CMAKE_MODULE_PATH}")
set(BUILD_STATIC ON)
set(BUILD_QT_SDL OFF)
FetchContent_MakeAvailable(melonDS libretro-common embed-binaries)