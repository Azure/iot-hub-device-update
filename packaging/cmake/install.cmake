# packaging/cmake/install.cmake
#
# CMake install rules for the ADU Gen2 agent packaging.
# Include this file from the top-level CMakeLists.txt:
#   include(packaging/cmake/install.cmake)
#
# GNU install directories are used so that paths adapt to the platform
# (e.g. lib vs lib64 on RPM distros).

include(GNUInstallDirs)

# ── Main agent binary ────────────────────────────────────────────────
if(TARGET adu_agent)
    install(TARGETS adu_agent
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# ── Extension shared libraries ───────────────────────────────────────
set(ADU_EXTENSIONS_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/adu/extensions")

set(_adu_extension_targets
    adu_direct_comm
    simulator_comm
    curl_downloader
    sideload_downloader
    script_handler
    swupdate_handler
    delta_processor
)

foreach(_ext IN LISTS _adu_extension_targets)
    if(TARGET ${_ext})
        install(TARGETS ${_ext}
            LIBRARY DESTINATION ${ADU_EXTENSIONS_INSTALL_DIR}
        )
    endif()
endforeach()

# ── Diagnostic tools ─────────────────────────────────────────────────
if(TARGET adu_status_cli)
    install(TARGETS adu_status_cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Install the rc_decoder wrapper script
install(PROGRAMS ${CMAKE_SOURCE_DIR}/packaging/tools/adu_rc_decoder
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    OPTIONAL
)

# Man pages
install(FILES
    ${CMAKE_SOURCE_DIR}/packaging/man/adu_status_cli.1
    ${CMAKE_SOURCE_DIR}/packaging/man/adu_rc_decoder.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
    OPTIONAL
)

# ── EDK: public headers ─────────────────────────────────────────────
file(GLOB _edk_headers "${CMAKE_SOURCE_DIR}/src/extension_sdk/inc/aduc/*.h")
install(FILES ${_edk_headers}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aduc
)

# ── EDK: CMake config files ─────────────────────────────────────────
install(FILES
    ${CMAKE_SOURCE_DIR}/packaging/cmake/ADUCEDKConfig.cmake
    ${CMAKE_SOURCE_DIR}/packaging/cmake/ADUCEDKConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ADUCEDK
)

# ── EDK: pkg-config file ────────────────────────────────────────────
configure_file(
    ${CMAKE_SOURCE_DIR}/packaging/cmake/aducedk.pc.in
    ${CMAKE_BINARY_DIR}/aducedk.pc
    @ONLY
)
install(FILES ${CMAKE_BINARY_DIR}/aducedk.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# ── EDK: Starter templates ──────────────────────────────────────────
install(DIRECTORY ${CMAKE_SOURCE_DIR}/packaging/templates/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/adu-edk/templates
    PATTERN "*.c"
    PATTERN "*.h"
    PATTERN "*.cmake"
    PATTERN "CMakeLists.txt"
)

# ── Systemd service unit ────────────────────────────────────────────
install(FILES ${CMAKE_SOURCE_DIR}/packaging/debian/adu-agent.service
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/system
)

# ── Default configuration ───────────────────────────────────────────
install(FILES ${CMAKE_SOURCE_DIR}/packaging/config/agent.toml
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/adu
)
