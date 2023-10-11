add_library(libretro-common STATIC)
target_include_directories(libretro-common PUBLIC "${libretro-common_SOURCE_DIR}/include")

target_sources(libretro-common PRIVATE
    ${libretro-common_SOURCE_DIR}/audio/conversion/float_to_s16.c
    ${libretro-common_SOURCE_DIR}/audio/conversion/s16_to_float.c
    ${libretro-common_SOURCE_DIR}/audio/resampler/audio_resampler.c
    ${libretro-common_SOURCE_DIR}/audio/resampler/drivers/sinc_resampler.c
    ${libretro-common_SOURCE_DIR}/compat/compat_fnmatch.c
    ${libretro-common_SOURCE_DIR}/compat/compat_posix_string.c
    ${libretro-common_SOURCE_DIR}/compat/compat_strldup.c
    ${libretro-common_SOURCE_DIR}/compat/fopen_utf8.c
    ${libretro-common_SOURCE_DIR}/encodings/encoding_base64.c
    ${libretro-common_SOURCE_DIR}/encodings/encoding_crc32.c
    ${libretro-common_SOURCE_DIR}/encodings/encoding_utf.c
    ${libretro-common_SOURCE_DIR}/features/features_cpu.c
    ${libretro-common_SOURCE_DIR}/file/archive_file.c
    ${libretro-common_SOURCE_DIR}/file/config_file.c
    ${libretro-common_SOURCE_DIR}/file/config_file_userdata.c
    ${libretro-common_SOURCE_DIR}/file/file_path.c
    ${libretro-common_SOURCE_DIR}/file/file_path_io.c
    ${libretro-common_SOURCE_DIR}/file/nbio/nbio_intf.c
    ${libretro-common_SOURCE_DIR}/file/nbio/nbio_stdio.c
    ${libretro-common_SOURCE_DIR}/file/retro_dirent.c
    ${libretro-common_SOURCE_DIR}/formats/bmp/rbmp_encode.c
    ${libretro-common_SOURCE_DIR}/formats/image_texture.c
    ${libretro-common_SOURCE_DIR}/formats/image_transfer.c
    ${libretro-common_SOURCE_DIR}/formats/json/rjson.c
    ${libretro-common_SOURCE_DIR}/formats/logiqx_dat/logiqx_dat.c
    ${libretro-common_SOURCE_DIR}/formats/m3u/m3u_file.c
    ${libretro-common_SOURCE_DIR}/formats/png/rpng.c
    ${libretro-common_SOURCE_DIR}/formats/png/rpng_encode.c
    ${libretro-common_SOURCE_DIR}/gfx/scaler/pixconv.c
    ${libretro-common_SOURCE_DIR}/gfx/scaler/scaler.c
    ${libretro-common_SOURCE_DIR}/gfx/scaler/scaler_filter.c
    ${libretro-common_SOURCE_DIR}/gfx/scaler/scaler_int.c
    ${libretro-common_SOURCE_DIR}/hash/lrc_hash.c
    ${libretro-common_SOURCE_DIR}/lists/dir_list.c
    ${libretro-common_SOURCE_DIR}/lists/file_list.c
    ${libretro-common_SOURCE_DIR}/lists/linked_list.c
    ${libretro-common_SOURCE_DIR}/lists/nested_list.c
    ${libretro-common_SOURCE_DIR}/lists/string_list.c
    ${libretro-common_SOURCE_DIR}/memmap/memalign.c
    ${libretro-common_SOURCE_DIR}/memmap/memmap.c
    ${libretro-common_SOURCE_DIR}/playlists/label_sanitization.c
    ${libretro-common_SOURCE_DIR}/queues/fifo_queue.c
    ${libretro-common_SOURCE_DIR}/queues/generic_queue.c
    ${libretro-common_SOURCE_DIR}/queues/message_queue.c
    ${libretro-common_SOURCE_DIR}/queues/task_queue.c
    ${libretro-common_SOURCE_DIR}/streams/file_stream.c
    ${libretro-common_SOURCE_DIR}/streams/file_stream_transforms.c
    ${libretro-common_SOURCE_DIR}/streams/interface_stream.c
    ${libretro-common_SOURCE_DIR}/streams/memory_stream.c
    ${libretro-common_SOURCE_DIR}/streams/network_stream.c
    ${libretro-common_SOURCE_DIR}/streams/rzip_stream.c
    ${libretro-common_SOURCE_DIR}/streams/stdin_stream.c
    ${libretro-common_SOURCE_DIR}/streams/trans_stream.c
    ${libretro-common_SOURCE_DIR}/streams/trans_stream_pipe.c
    ${libretro-common_SOURCE_DIR}/string/stdstring.c
    ${libretro-common_SOURCE_DIR}/time/rtime.c
    ${libretro-common_SOURCE_DIR}/utils/md5.c
    ${libretro-common_SOURCE_DIR}/vfs/vfs_implementation.c
    )

add_common_definitions(libretro-common)
target_include_directories(libretro-common PRIVATE ${CMAKE_SOURCE_DIR}/src/libretro)
target_compile_definitions(libretro-common PRIVATE HAVE_GLSYM_PRIVATE HAVE_RPNG)

if (HAVE_DYNAMIC)
    target_sources(libretro-common PRIVATE
        ${libretro-common_SOURCE_DIR}/dynamic/dylib.c
    )
endif ()

if (HAVE_EGL)
    target_link_libraries(libretro-common PUBLIC OpenGL::EGL)
endif ()

if (HAVE_NETWORKING)
    target_sources(libretro-common PRIVATE
        ${libretro-common_SOURCE_DIR}/net/net_compat.c
        ${libretro-common_SOURCE_DIR}/net/net_http.c
        ${libretro-common_SOURCE_DIR}/net/net_http_parse.c
        ${libretro-common_SOURCE_DIR}/net/net_socket.c
        )
endif ()

if (HAVE_OPENGL OR HAVE_OPENGLES)
    target_sources(libretro-common PRIVATE
        ${libretro-common_SOURCE_DIR}/gfx/gl_capabilities.c
        ${libretro-common_SOURCE_DIR}/glsm/glsm.c
        ${libretro-common_SOURCE_DIR}/glsym/rglgen.c
        )
endif()

if (HAVE_OPENGL)
    target_sources(libretro-common PRIVATE ${libretro-common_SOURCE_DIR}/glsym/glsym_gl.c)
    target_link_libraries(libretro-common PUBLIC OpenGL::GL)
    target_include_directories(libretro-common PUBLIC SYSTEM ${OPENGL_INCLUDE_DIR})
endif ()

if (HAVE_OPENGLES2)
    target_sources(libretro-common PRIVATE ${libretro-common_SOURCE_DIR}/glsym/glsym_es2.c)
    target_include_directories(libretro-common PUBLIC SYSTEM ${OpenGLES_V2_INCLUDE_DIR})
endif ()

if (HAVE_OPENGLES3 OR HAVE_OPENGLES31 OR HAVE_OPENGLES32)
    target_sources(libretro-common PRIVATE ${libretro-common_SOURCE_DIR}/glsym/glsym_es3.c)
endif()

if (HAVE_OPENGLES31)
    target_include_directories(libretro-common PUBLIC SYSTEM ${OpenGLES_V31_INCLUDE_DIR})
elseif (HAVE_OPENGLES32)
    target_include_directories(libretro-common PUBLIC SYSTEM ${OpenGLES_V32_INCLUDE_DIR})
else()
    target_include_directories(libretro-common PUBLIC SYSTEM ${OpenGLES_V3_INCLUDE_DIR})
endif()

if (NOT HAVE_STRL)
    target_sources(libretro-common PRIVATE
        ${libretro-common_SOURCE_DIR}/compat/compat_strl.c
        ${libretro-common_SOURCE_DIR}/compat/compat_strldup.c
        )
endif ()

if (HAVE_THREADS)
    target_sources(libretro-common PRIVATE
        ${libretro-common_SOURCE_DIR}/rthreads/rthreads.c
        )
endif ()

if (HAVE_ZLIB)
    target_sources(libretro-common PRIVATE
        ${libretro-common_SOURCE_DIR}/file/archive_file_zlib.c
        ${libretro-common_SOURCE_DIR}/streams/trans_stream_zlib.c
    )
    target_link_libraries(libretro-common PUBLIC zlibstatic)
    target_include_directories(libretro-common SYSTEM PUBLIC ${zlib_SOURCE_DIR})
    target_include_directories(libretro-common PUBLIC ${zlib_BINARY_DIR})
endif ()

set_target_properties(libretro-common PROPERTIES PREFIX "" OUTPUT_NAME "libretro-common")