set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(embed-binaries)

add_library(melondsds_libretro MODULE
    buffer.cpp
    buffer.hpp
    config/config.hpp
    config/config.cpp
    config/console.hpp
    config/console.cpp
    config/constants.cpp
    config/constants.hpp
    config/definitions.hpp
    config/definitions/audio.hpp
    config/definitions/categories.hpp
    config/definitions/cpu.hpp
    config/definitions/firmware.hpp
    config/definitions/network.hpp
    config/definitions/osd.hpp
    config/definitions/screen.hpp
    config/definitions/system.hpp
    config/definitions/video.hpp
    config/parse.cpp
    config/parse.hpp
    config/types.hpp
    config/visibility.hpp
    config/visibility.cpp
    core/core.cpp
    core/core.hpp
    core/tasks.cpp
    core/test.cpp
    core/test.hpp
    environment.cpp
    environment.hpp
    exceptions.cpp
    exceptions.hpp
    format.cpp
    format.hpp
    glsym_private.cpp
    glsym_private.h
    info.cpp
    info.hpp
    input.cpp
    input.hpp
    libretro.cpp
    libretro.hpp
    math.hpp
    message/error.cpp
    message/error.hpp
    microphone.cpp
    microphone.hpp
    pcap.hpp
    platform/file.cpp
    platform/lan.cpp
    platform/mp.cpp
    platform/mutex.cpp
    platform/platform.cpp
    platform/semaphore.cpp
    platform/thread.cpp
    PlatformOGLPrivate.h
    render/render.cpp
    render/render.hpp
    render/software.cpp
    render/software.hpp
    retro/dirent.cpp
    retro/dirent.hpp
    retro/http.cpp
    retro/http.hpp
    retro/info.cpp
    retro/info.hpp
    retro/microphone.cpp
    retro/microphone.hpp
    retro/scaler.cpp
    retro/scaler.hpp
    retro/task_queue.cpp
    retro/task_queue.hpp
    retro/threads.cpp
    retro/threads.hpp
    screenlayout.cpp
    screenlayout.hpp
    std/chrono.hpp
    std/semaphore.hpp
    std/span.hpp
    sram.cpp
    sram.hpp
    tracy.hpp
    utils.cpp
    utils.hpp
    ../pntr/pntr.c
)

target_include_directories(melondsds_libretro SYSTEM PUBLIC
    "${libretro-common_SOURCE_DIR}/include"
    "${melonDS_SOURCE_DIR}/src"
    "${glm_SOURCE_DIR}"
    "${pntr_SOURCE_DIR}"
    "${fmt_SOURCE_DIR}/include"
    "${yamc_SOURCE_DIR}/include"
    "${span-lite_SOURCE_DIR}/include"
    "${date_SOURCE_DIR}/include"
    )

target_include_directories(melondsds_libretro PRIVATE "${CMAKE_SOURCE_DIR}/src/libretro")

embed_binaries(libretro-assets
    ASSET
        NAME "melondsds_vertex_shader"
        PATH "glsl/melondsds.vert"
        BYTE_TYPE char
        NULL_TERMINATE
    ASSET
        NAME "melondsds_fragment_shader"
        PATH "glsl/melondsds.frag"
        BYTE_TYPE char
        NULL_TERMINATE
    ASSET
        NAME "melondsds_default_wfc_config"
        PATH "assets/wfc.cfg"
        BYTE_TYPE char
        NULL_TERMINATE
    ASSET
        NAME "melondsds_graphic_error"
        PATH "assets/melon-error.png"
    ASSET
        NAME "melondsds_graphic_sorry"
        PATH "assets/melon-sorry.png"
    ASSET
        NAME "melondsds_error_title_font"
        PATH "assets/OpenSans-ExtraBold.ttf"
    ASSET
        NAME "melondsds_error_body_font"
        PATH "assets/OpenSans_Condensed-SemiBold.ttf"
)

add_common_definitions(melondsds_libretro)

if (ENABLE_THREADED_RENDERER)
    target_compile_definitions(melondsds_libretro PUBLIC ENABLE_THREADED_RENDERER HAVE_THREADED_RENDERER)
endif ()

if (HAVE_NETWORKING)
    target_sources(melondsds_libretro PRIVATE
        ${melonDS_SOURCE_DIR}/src/frontend/qt_sdl/LAN_Socket.cpp
        platform/lan.cpp
    )

    target_include_directories(melondsds_libretro SYSTEM PRIVATE "${melonDS_SOURCE_DIR}/src/frontend/qt_sdl")

    if (HAVE_NETWORKING_DIRECT_MODE)
        target_sources(melondsds_libretro PRIVATE
            ${melonDS_SOURCE_DIR}/src/frontend/qt_sdl/LAN_PCap.cpp
        )
    endif()
endif ()

if (TRACY_ENABLE)
    target_sources(melondsds_libretro PRIVATE tracy.cpp)
endif ()

if (HAVE_OPENGL OR HAVE_OPENGLES)
    target_sources(melondsds_libretro PRIVATE
        render/opengl.cpp
        render/opengl.hpp
    )
endif()

if (HAVE_OPENGL)
    if (APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-framework,OpenGL")
    endif ()
    target_link_libraries(melondsds_libretro PUBLIC OpenGL::GL)
    target_include_directories(melondsds_libretro SYSTEM PUBLIC "${OPENGL_INCLUDE_DIR}")
elseif (HAVE_OPENGLES1)
    target_link_libraries(melondsds_libretro PUBLIC OpenGLES::OpenGLESv1)
    target_include_directories(melondsds_libretro SYSTEM PUBLIC "${OpenGLES_V1_INCLUDE_DIR}")
elseif (HAVE_OPENGLES2)
    target_link_libraries(melondsds_libretro PUBLIC OpenGLES::OpenGLESv2)
    target_include_directories(melondsds_libretro SYSTEM PUBLIC "${OpenGLES_V2_INCLUDE_DIR}")
elseif (HAVE_OPENGLES3)
    target_link_libraries(melondsds_libretro PUBLIC OpenGLES::OpenGLESv3)
    target_include_directories(melondsds_libretro SYSTEM PUBLIC "${OpenGLES_V3_INCLUDE_DIR}")
elseif (HAVE_OPENGLES31)
    target_link_libraries(melondsds_libretro PUBLIC OpenGLES::OpenGLESv31)
    target_include_directories(melondsds_libretro SYSTEM PUBLIC "${OpenGLES_V31_INCLUDE_DIR}")
elseif (HAVE_OPENGLES32)
    target_link_libraries(melondsds_libretro PUBLIC OpenGLES::OpenGLESv32)
    target_include_directories(melondsds_libretro SYSTEM PUBLIC "${OpenGLES_V32_INCLUDE_DIR}")
endif ()

if (ENABLE_JIT)
    target_compile_definitions(melondsds_libretro PUBLIC JIT_ENABLED HAVE_JIT ENABLE_JIT)

    if (NOT APPLE)
        target_compile_definitions(melondsds_libretro PUBLIC HAVE_JIT_FASTMEM)
        # melonDS doesn't support FastMem on macOS
    endif ()
endif ()

# libretro cores do not start with "lib"
set_target_properties(melondsds_libretro PROPERTIES PREFIX "")

# Some platforms or compilers don't use the expected suffixes for shared libraries
if (APPLE)
    set_target_properties(melondsds_libretro PROPERTIES SUFFIX ".dylib")
elseif(ANDROID)
    set_target_properties(melondsds_libretro PROPERTIES SUFFIX "_android.so")
elseif (UNIX)
    set_target_properties(melondsds_libretro PROPERTIES SUFFIX ".so")
elseif (WIN32)
    set_target_properties(melondsds_libretro PROPERTIES SUFFIX ".dll")
endif ()

if (WIN32 AND MINGW)
    target_link_options(melondsds_libretro PUBLIC -static-libgcc -static-libstdc++ -static)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "(.+)?Clang")
    target_compile_options(melondsds_libretro PUBLIC -Werror=return-type)
    # For some reason, C++ allows functions to not return values in all code paths.
    # This has tripped me up before, so I'm forcing it to be an error.
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/version.hpp")

target_compile_definitions(melondsds_libretro PUBLIC
    GLM_ENABLE_EXPERIMENTAL
    PNTR_ENABLE_DEFAULT_FONT
    PNTR_ENABLE_TTF
    PNTR_ENABLE_VARGS
    PNTR_PIXELFORMAT_ARGB
    PNTR_ENABLE_MATH
    PNTR_ENABLE_UTF8
    PNTR_NO_STDIO
    PNTR_NO_SAVE_IMAGE
    STB_IMAGE_STATIC # To avoid using any dynamically-linked STB implementations
)

target_link_libraries(melondsds_libretro PUBLIC libretro-common core libretro-assets glm::glm pntr fmt::fmt date)

if (HAVE_NETWORKING)
    target_compile_definitions(melondsds_libretro PUBLIC BUILDING_LIBSLIRP)
    target_link_libraries(melondsds_libretro PUBLIC slirp)

    if (WIN32)
        target_link_libraries(melondsds_libretro PRIVATE ws2_32 iphlpapi)
    endif()
endif()

if (TRACY_ENABLE)
    target_link_libraries(melondsds_libretro PUBLIC TracyClient)
    target_include_directories(melondsds_libretro SYSTEM PUBLIC TracyClient)
    target_compile_definitions(melondsds_libretro PUBLIC HAVE_TRACY)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Defining DEBUG in melondsds_libretro and libretro-common targets")
    target_compile_definitions(melondsds_libretro PUBLIC DEBUG)
    target_compile_definitions(libretro-common PUBLIC DEBUG)
endif ()