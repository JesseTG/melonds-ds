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
    opengl.cpp
    platform/mutex.cpp
    platform/semaphore.cpp
    platform/thread.cpp
    screenlayout.cpp
    platform/file.cpp)

target_include_directories(libretro SYSTEM PUBLIC
    "${libretro-common_SOURCE_DIR}/include"
    "${melonDS_SOURCE_DIR}/src"
    )
target_compile_definitions(libretro PUBLIC JIT_ENABLED HAVE_OPENGL OGLRENDERER_ENABLED HAVE_THREADS PLATFORMOGL_H) # TODO: Disable on certain platforms
set_target_properties(libretro PROPERTIES PREFIX "" OUTPUT_NAME "melondsds_libretro")
# Name the output library "melondsds_libretro.dll" instead of "libmelondsds_libretro.dll", as is the convention

target_link_libraries(libretro PUBLIC libretro-common core)
