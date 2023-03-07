set (CMAKE_CXX_STANDARD 17)

add_library(libretro STATIC
    libretro.cpp environment.cpp info.cpp config.cpp)

target_include_directories(libretro SYSTEM PUBLIC "${libretro-common_SOURCE_DIR}/include")
target_compile_definitions(libretro PUBLIC JIT_ENABLED HAVE_OPENGL HAVE_THREADS) # TODO: Disable on certain platforms