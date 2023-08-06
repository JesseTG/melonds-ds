set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(embed-binaries)

add_library(libretro MODULE
    "${melonDS_SOURCE_DIR}/src/frontend/Util_Audio.cpp"
    buffer.cpp
    buffer.hpp
    config.cpp
    config.hpp
    content.cpp
    content.hpp
    dsi.cpp
    dsi.hpp
    environment.cpp
    environment.hpp
    exceptions.cpp
    exceptions.hpp
    fat.cpp
    fat.hpp
    gba.cpp
    gba.hpp
    glsym_private.cpp
    glsym_private.h
    info.cpp
    info.hpp
    input.cpp
    input.hpp
    libretro.cpp
    libretro.hpp
    math.hpp
    microphone.cpp
    microphone.hpp
    memory.cpp
    memory.hpp
    platform/config.cpp
    platform/file.cpp
    platform/lan.cpp
    platform/mp.cpp
    platform/mutex.cpp
    platform/platform.cpp
    platform/semaphore.cpp
    platform/thread.cpp
    PlatformOGLPrivate.h
    power.cpp
    power.hpp
    render.cpp
    render.hpp
    retro/task_queue.cpp
    retro/task_queue.hpp
    screenlayout.cpp
    screenlayout.hpp
    tracy.hpp
    utils.cpp
    utils.hpp
)

target_include_directories(libretro SYSTEM PUBLIC
    "${libretro-common_SOURCE_DIR}/include"
    "${melonDS_SOURCE_DIR}/src"
    "${glm_SOURCE_DIR}"
    )

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
)

add_common_definitions(libretro)

if (HAVE_NETWORKING)
    target_sources(libretro PRIVATE
        ${melonDS_SOURCE_DIR}/src/frontend/qt_sdl/LAN_PCap.cpp
        ${melonDS_SOURCE_DIR}/src/frontend/qt_sdl/LAN_Socket.cpp
        platform/lan.cpp
    )

    target_include_directories(libretro SYSTEM PRIVATE "${melonDS_SOURCE_DIR}/src/frontend/qt_sdl")
endif ()

if (HAVE_OPENGL OR HAVE_OPENGLES)
    target_sources(libretro PRIVATE opengl.cpp)
endif()

if (HAVE_OPENGL)
    if (APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-framework,OpenGL")
    endif ()
    target_link_libraries(libretro PUBLIC OpenGL::GL)
    target_include_directories(libretro SYSTEM PUBLIC "${OPENGL_INCLUDE_DIR}")
elseif (HAVE_OPENGLES1)
    target_link_libraries(libretro PUBLIC OpenGLES::OpenGLESv1)
    target_include_directories(libretro SYSTEM PUBLIC "${OpenGLES_V1_INCLUDE_DIR}")
elseif (HAVE_OPENGLES2)
    target_link_libraries(libretro PUBLIC OpenGLES::OpenGLESv2)
    target_include_directories(libretro SYSTEM PUBLIC "${OpenGLES_V2_INCLUDE_DIR}")
elseif (HAVE_OPENGLES3)
    target_link_libraries(libretro PUBLIC OpenGLES::OpenGLESv3)
    target_include_directories(libretro SYSTEM PUBLIC "${OpenGLES_V3_INCLUDE_DIR}")
elseif (HAVE_OPENGLES31)
    target_link_libraries(libretro PUBLIC OpenGLES::OpenGLESv31)
    target_include_directories(libretro SYSTEM PUBLIC "${OpenGLES_V31_INCLUDE_DIR}")
elseif (HAVE_OPENGLES32)
    target_link_libraries(libretro PUBLIC OpenGLES::OpenGLESv32)
    target_include_directories(libretro SYSTEM PUBLIC "${OpenGLES_V32_INCLUDE_DIR}")
endif ()

if (ENABLE_JIT)
    target_compile_definitions(libretro PUBLIC JIT_ENABLED HAVE_JIT ENABLE_JIT)

    if (NOT APPLE)
        target_compile_definitions(libretro PUBLIC HAVE_JIT_FASTMEM)
        # melonDS doesn't support FastMem on macOS
    endif ()
endif ()

if (HAVE_THREADS)
    target_sources(libretro PRIVATE
        ../rthreads/rsemaphore.c
        ../rthreads/rsemaphore.h
        )
endif ()

# libretro cores do not start with "lib"
set_target_properties(libretro PROPERTIES PREFIX "")

# Most platforms use the name "<core name>_libretro", but a few have their own conventions
if (ANDROID)
    set_target_properties(libretro PROPERTIES OUTPUT_NAME "melondsds_libretro_android")
elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    set_target_properties(libretro PROPERTIES OUTPUT_NAME "melondsds_libretro_tvos")
elseif(IOS)
    set_target_properties(libretro PROPERTIES OUTPUT_NAME "melondsds_libretro_ios")
else()
    set_target_properties(libretro PROPERTIES OUTPUT_NAME "melondsds_libretro")
endif()

# Some platforms or compilers don't use the expected suffixes for shared libraries
if (APPLE)
    set_target_properties(libretro PROPERTIES SUFFIX ".dylib")
elseif (UNIX)
    set_target_properties(libretro PROPERTIES SUFFIX ".so")
elseif (WIN32)
    set_target_properties(libretro PROPERTIES SUFFIX ".dll")
endif ()

if (WIN32 AND MINGW)
    target_link_options(libretro PUBLIC -static-libgcc -static-libstdc++ -static)
endif()

target_compile_definitions(libretro PUBLIC MELONDSDS_VERSION="${CMAKE_PROJECT_VERSION}")
target_compile_definitions(libretro PUBLIC MELONDSDS_NAME="${CMAKE_PROJECT_NAME}")

target_link_libraries(libretro PUBLIC libretro-common core libretro-assets glm::glm_static)

if (HAVE_NETWORKING)
    target_compile_definitions(libretro PUBLIC BUILDING_LIBSLIRP)
    target_link_libraries(libretro PUBLIC slirp)

    if (WIN32)
        target_link_libraries(libretro PRIVATE ws2_32 iphlpapi)
    endif()
endif()

if (TRACY_ENABLE)
    target_link_libraries(libretro PUBLIC TracyClient)
    target_include_directories(libretro SYSTEM PUBLIC TracyClient)
    target_compile_definitions(libretro PUBLIC HAVE_TRACY)
endif()