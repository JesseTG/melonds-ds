set (CMAKE_CXX_STANDARD 17)

add_library(libretro STATIC
    libretro.cpp)

target_include_directories(libretro SYSTEM PUBLIC "${libretro-common_SOURCE_DIR}/include")