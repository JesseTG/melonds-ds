file(READ "${CMAKE_SOURCE_DIR}/LICENSE" MELONDSDS_LICENSE)
file(READ "${melonDS_SOURCE_DIR}/LICENSE" MELONDS_LICENSE)
file(READ "${melonDS_SOURCE_DIR}/src/dolphin/license_dolphin.txt" DOLPHIN_LICENSE)
file(READ "${melonDS_SOURCE_DIR}/src/fatfs/LICENSE.txt" FATFS_LICENSE)
file(READ "${melonDS_SOURCE_DIR}/src/teakra/LICENSE" TEAKRA_LICENSE)
file(READ "${melonDS_SOURCE_DIR}/freebios/drastic_bios_readme.txt" FREEBIOS_LICENSE)
file(READ "${fmt_SOURCE_DIR}/LICENSE.rst" FMT_LICENSE)
file(READ "${glm_SOURCE_DIR}/copying.txt" GLM_LICENSE)
file(READ "${pntr_SOURCE_DIR}/LICENSE" PNTR_LICENSE)
file(READ "${yamc_SOURCE_DIR}/LICENSE" YAMC_LICENSE)
file(READ "${libslirp_SOURCE_DIR}/COPYRIGHT" SLIRP_LICENSE)
file(READ "${span-lite_SOURCE_DIR}/LICENSE.txt" SPAN_LITE_LICENSE)
file(READ "${zlib_SOURCE_DIR}/LICENSE" ZLIB_LICENSE)

configure_file("${CMAKE_SOURCE_DIR}/cmake/melondsds-LICENSE.txt.in" "${CMAKE_CURRENT_BINARY_DIR}/melondsds-LICENSE.txt")

# TODO: Conditionally add licenses for pcap and tracy