set (CMAKE_CXX_STANDARD 17)

add_library(libretro SHARED
    libretro.cpp
    environment.cpp
    info.cpp config.cpp
    screenlayout.cpp
    platform/thread.cpp
    platform/semaphore.cpp
    platform/mutex.cpp
    ../rthreads/rsemaphore.c
    input.cpp input.hpp opengl.cpp opengl.hpp)

target_include_directories(libretro SYSTEM PUBLIC
    "${libretro-common_SOURCE_DIR}/include"
    "${melonDS_SOURCE_DIR}/src"
    )
target_compile_definitions(libretro PUBLIC JIT_ENABLED HAVE_OPENGL HAVE_THREADS) # TODO: Disable on certain platforms
# TODO: Set the target's name