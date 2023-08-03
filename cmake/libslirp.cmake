add_library(slirp STATIC
        ../src/glib-stub/glib.c
)
target_include_directories(slirp PUBLIC
    "${libslirp_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/glib-stub"
)
target_include_directories(slirp SYSTEM PUBLIC
    "${CMAKE_SOURCE_DIR}/src/glib-stub"
    "${libretro-common_SOURCE_DIR}/include"
)

target_sources(slirp PRIVATE
    "${libslirp_SOURCE_DIR}/src/arp_table.c"
    "${libslirp_SOURCE_DIR}/src/bootp.c"
    "${libslirp_SOURCE_DIR}/src/cksum.c"
    "${libslirp_SOURCE_DIR}/src/dhcpv6.c"
    "${libslirp_SOURCE_DIR}/src/dnssearch.c"
    "${libslirp_SOURCE_DIR}/src/if.c"
    "${libslirp_SOURCE_DIR}/src/ip_icmp.c"
    "${libslirp_SOURCE_DIR}/src/ip_input.c"
    "${libslirp_SOURCE_DIR}/src/ip_output.c"
    "${libslirp_SOURCE_DIR}/src/ip6_icmp.c"
    "${libslirp_SOURCE_DIR}/src/ip6_input.c"
    "${libslirp_SOURCE_DIR}/src/ip6_output.c"
    "${libslirp_SOURCE_DIR}/src/mbuf.c"
    "${libslirp_SOURCE_DIR}/src/ndp_table.c"
    "${libslirp_SOURCE_DIR}/src/ncsi.c"
    "${libslirp_SOURCE_DIR}/src/sbuf.c"
    "${libslirp_SOURCE_DIR}/src/slirp.c"
    "${libslirp_SOURCE_DIR}/src/socket.c"
    "${libslirp_SOURCE_DIR}/src/tcp_input.c"
    "${libslirp_SOURCE_DIR}/src/tcp_output.c"
    "${libslirp_SOURCE_DIR}/src/tcp_subr.c"
    "${libslirp_SOURCE_DIR}/src/tcp_timer.c"
    "${libslirp_SOURCE_DIR}/src/tftp.c"
    "${libslirp_SOURCE_DIR}/src/udp.c"
    "${libslirp_SOURCE_DIR}/src/udp6.c"
    "${libslirp_SOURCE_DIR}/src/util.c"
)

target_compile_definitions(slirp PRIVATE BUILDING_LIBSLIRP)

if (WIN32)
    target_link_libraries(slirp PRIVATE ws2_32 iphlpapi)
endif()

if (UNIX)
    target_compile_definitions(slirp PRIVATE UNIX)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(slirp PRIVATE -fPIC)
endif ()
