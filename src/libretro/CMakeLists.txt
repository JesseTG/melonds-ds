set(CMAKE_CXX_STANDARD 17)

add_library(libretro MODULE
    "${melonDS_SOURCE_DIR}/src/frontend/Util_Audio.cpp"
    ../rthreads/rsemaphore.c
    config.cpp
    environment.cpp
    info.cpp
    input.cpp
    libretro.cpp
    memory.cpp
    platform/camera.cpp
    platform/config.cpp
    platform/file.cpp
    platform/lan.cpp
    platform/mp.cpp
    platform/mutex.cpp
    platform/platform.cpp
    platform/semaphore.cpp
    platform/thread.cpp
    screenlayout.cpp
    )


if (HAVE_OPENGL)
    target_sources(libretro PRIVATE opengl.cpp)
endif ()

target_include_directories(libretro SYSTEM PUBLIC
    "${libretro-common_SOURCE_DIR}/include"
    "${melonDS_SOURCE_DIR}/src"
    )

if (HAVE_STRL)
    target_compile_definitions(libretro PUBLIC HAVE_STRL)
endif ()

if (HAVE_THREADS)
    target_compile_definitions(libretro PUBLIC HAVE_THREADS)
endif ()

if (HAVE_OPENGL)
    target_compile_definitions(libretro PUBLIC HAVE_OPENGL OGLRENDERER_ENABLED ENABLE_OGLRENDERER PLATFORMOGL_H)
    if (APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-framework,OpenGL")
    endif ()
    target_link_libraries(libretro PUBLIC OpenGL::GL)
endif ()

if (APPLE)
    target_compile_definitions(libretro PUBLIC GL_SILENCE_DEPRECATION)
    # macOS has deprecated OpenGL, and its headers spit out a lot of warnings
endif()

target_compile_definitions(libretro PUBLIC JIT_ENABLED) # TODO: Disable on certain platforms
set_target_properties(libretro PROPERTIES PREFIX "" OUTPUT_NAME "melondsds_libretro")

if (APPLE)
    set_target_properties(libretro PROPERTIES SUFFIX ".dylib")
elseif (UNIX)
    set_target_properties(libretro PROPERTIES SUFFIX ".so")
elseif (WIN32)
    set_target_properties(libretro PROPERTIES SUFFIX ".dll")
endif ()
# Name the output library "melondsds_libretro.XXX" instead of "libmelondsds_libretro.XXX", as is the convention for libretro

target_link_libraries(libretro PUBLIC libretro-common core)
