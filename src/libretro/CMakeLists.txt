set(CMAKE_CXX_STANDARD 17)

add_library(libretro MODULE
    "${melonDS_SOURCE_DIR}/src/frontend/Util_Audio.cpp"
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
    info.cpp
    info.hpp
    input.cpp
    input.hpp
    libretro.cpp
    libretro.hpp
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
    render.cpp
    render.hpp
    screenlayout.cpp
    screenlayout.hpp
    utils.cpp
    utils.hpp)

target_include_directories(libretro SYSTEM PUBLIC
    "${libretro-common_SOURCE_DIR}/include"
    "${melonDS_SOURCE_DIR}/src"
    )

add_common_definitions(libretro)

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
    target_compile_definitions(libretro PUBLIC JIT_ENABLED HAVE_JIT ENABLE_JIT)  # TODO: Disable on certain platforms

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

set_target_properties(libretro PROPERTIES PREFIX "" OUTPUT_NAME "melondsds_libretro")

if (APPLE)
    set_target_properties(libretro PROPERTIES SUFFIX ".dylib")
elseif (UNIX)
    set_target_properties(libretro PROPERTIES SUFFIX ".so")
elseif (WIN32)
    set_target_properties(libretro PROPERTIES SUFFIX ".dll")
endif ()
# Name the output library "melondsds_libretro.XXX" instead of "libmelondsds_libretro.XXX", as is the convention for libretro

if (WIN32 AND MINGW)
    target_link_options(libretro PUBLIC -static-libgcc -static-libstdc++ -static)
endif()

target_compile_definitions(libretro PUBLIC MELONDSDS_VERSION="${CMAKE_PROJECT_VERSION}")
target_compile_definitions(libretro PUBLIC MELONDSDS_NAME="${CMAKE_PROJECT_NAME}")

target_link_libraries(libretro PUBLIC libretro-common core)
