include(cmake/libretro-common.cmake)
include(cmake/libslirp.cmake)
include(cmake/tinyexpr-plusplus.cmake)

if (HAVE_OPENGL OR HAVE_OPENGLES)
    # Ensure that melonDS can find libretro's headers...
    target_include_directories(core SYSTEM PRIVATE "${libretro-common_SOURCE_DIR}/include")
    target_include_directories(core PRIVATE "${CMAKE_SOURCE_DIR}/src/libretro")

    if (HAVE_OPENGL)
        target_include_directories(core SYSTEM PRIVATE "${OPENGL_INCLUDE_DIR}")
    endif ()

    if (HAVE_OPENGLES1)
        target_include_directories(core SYSTEM PRIVATE "${OpenGLES_V1_INCLUDE_DIR}")
    endif ()

    if (HAVE_OPENGLES2)
        target_include_directories(core SYSTEM PRIVATE "${OpenGLES_V2_INCLUDE_DIR}")
    endif ()

    if (HAVE_OPENGLES3)
        target_include_directories(core SYSTEM PRIVATE "${OpenGLES_V3_INCLUDE_DIR}")
    endif ()

    if (HAVE_OPENGLES31)
        target_include_directories(core SYSTEM PRIVATE "${OpenGLES_V31_INCLUDE_DIR}")
    endif ()

    if (HAVE_OPENGLES32)
        target_include_directories(core SYSTEM PRIVATE "${OpenGLES_V32_INCLUDE_DIR}")
    endif ()

    target_compile_definitions(core PUBLIC OGLRENDERER_ENABLED ENABLE_OGLRENDERER)
endif ()

add_common_definitions(core)

if (HAVE_NETWORKING)
    # Ensure that the visibility attributes are defined
    target_compile_definitions(core PUBLIC BUILDING_LIBSLIRP)
endif()