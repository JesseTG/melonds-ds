set (CMAKE_CXX_STANDARD 17)

add_library(libretro MODULE
    "${melonDS_SOURCE_DIR}/src/frontend/Util_Audio.cpp"
    ../rthreads/rsemaphore.c
    config.cpp
    environment.cpp
    info.cpp
    input.cpp
    libretro.cpp
    memory.cpp
    platform/mutex.cpp
    platform/semaphore.cpp
    platform/thread.cpp
    platform/config.cpp
    screenlayout.cpp
    platform/file.cpp)

if (HAVE_OPENGL)
    target_sources(libretro PRIVATE opengl.cpp)
endif()

target_include_directories(libretro SYSTEM PUBLIC
    "${libretro-common_SOURCE_DIR}/include"
    "${melonDS_SOURCE_DIR}/src"
    )

if (HAVE_STRL)
    target_compile_definitions(libretro PUBLIC HAVE_STRL)
endif()

if (HAVE_THREADS)
    target_compile_definitions(libretro PUBLIC HAVE_THREADS)
endif()

if (HAVE_OPENGL)
    target_compile_definitions(libretro PUBLIC HAVE_OPENGL OGLRENDERER_ENABLED ENABLE_OGLRENDERER PLATFORMOGL_H)
    if (APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-framework,OpenGL")
    endif ()
    target_link_libraries(libretro PUBLIC OpenGL::GL)
endif ()

target_compile_definitions(libretro PUBLIC JIT_ENABLED) # TODO: Disable on certain platforms
set_target_properties(libretro PROPERTIES PREFIX "" OUTPUT_NAME "melondsds_libretro")
# Name the output library "melondsds_libretro.dll" instead of "libmelondsds_libretro.dll", as is the convention

target_link_libraries(libretro PUBLIC libretro-common core)
