include(cmake/libretro-common.cmake)
include(cmake/libslirp.cmake)
include(cmake/stb.cmake)

if (HAVE_OPENGL OR HAVE_OPENGLES)
    # Upstream melonDS uses GLAD to load OpenGL, but we want to use libretro's loader.
    # So instead of patching melonDS, let's change how it's compiled.

    file(GLOB melonDS_cpp_sources "${melonDS_SOURCE_DIR}/src/*.cpp")
    # Get all C++ source files within melonDS' main directory.
    # Need to specify C++ files because melonDS uses assembly,
    # which chokes if the glsym headers are included.

    # Now ensure that melonDS can find libretro's headers...
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

    add_common_definitions(core)

    # Ensure that the header that includes GLAD is *excluded* (PLATFORMOGL_H),
    # and that melonDS itself uses OpenGL (or OpenGL ES).
    target_compile_definitions(core PUBLIC PLATFORMOGL_H OGLRENDERER_ENABLED ENABLE_OGLRENDERER)

    # Now let's include the relevant header before each melonDS C++ source file.
    set_source_files_properties(
        ${melonDS_cpp_sources}
        TARGET_DIRECTORY
        core
        PROPERTIES
        COMPILE_OPTIONS "-include;PlatformOGLPrivate.h")
    # TODO: Adapt for GLES2 and GLES3
endif ()

if (HAVE_NETWORKING)
    # Ensure that the visibility attributes are defined
    target_compile_definitions(core PUBLIC BUILDING_LIBSLIRP)
endif()