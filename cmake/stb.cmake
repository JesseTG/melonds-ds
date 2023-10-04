add_library(stb STATIC
    ../src/stb/stb_truetype.c
)

target_include_directories(stb SYSTEM PUBLIC
    "${stb_SOURCE_DIR}"
)