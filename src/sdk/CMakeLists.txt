
set (target_name aducsdk)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})
add_library (aduc::sdk ALIAS ${target_name})

target_sources(${target_name} PRIVATE src/aducsdk.c)

target_include_directories (${target_name}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${target_name} PRIVATE aduc::apiproto_utils)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_API_DEFAULT_FIFO_PATH="${ADUC_API_DEFAULT_FIFO_PATH}"
                           ADUC_DATA_FOLDER="${ADUC_DATA_FOLDER}")

include(GNUInstallDirs)

install(TARGETS ${target_name}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY inc/aduc
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/aducsdk.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/aducsdk.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/aducsdk.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
