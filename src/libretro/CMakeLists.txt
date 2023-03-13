set (CMAKE_CXX_STANDARD 17)

add_library(libretro SHARED
    ../rthreads/rsemaphore.c
    config.cpp
    environment.cpp
    info.cpp
    input.cpp
    libretro.cpp
    memory.cpp
    opengl.cpp
    platform/mutex.cpp
    platform/semaphore.cpp
    platform/thread.cpp
    screenlayout.cpp
    )

target_include_directories(libretro SYSTEM PUBLIC
    "${libretro-common_SOURCE_DIR}/include"
    "${melonDS_SOURCE_DIR}/src"
    )
target_compile_definitions(libretro PUBLIC JIT_ENABLED HAVE_OPENGL OGLRENDERER_ENABLED HAVE_THREADS) # TODO: Disable on certain platforms
# TODO: Set the target's name