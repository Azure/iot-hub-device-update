
set (target_name aducsdk)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})
add_library (aduc::sdk ALIAS ${target_name})

target_sources(${target_name} PRIVATE src/aducsdk.c)

if (NOT WIN32)
    # Include apiproto source directly to avoid logging dependency
    # We compile it with ADUC_APIPROTO_NO_LOGGING to disable logging
    target_sources(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/src/utils/apiproto_utils/src/apiproto.c)
endif ()

target_include_directories (${target_name}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/utils/c_utils/inc
)

if (NOT WIN32)
    target_include_directories (${target_name}
        PRIVATE ${CMAKE_SOURCE_DIR}/src/utils/apiproto_utils/inc
    )
endif ()

# Link only c_utils, not apiproto_utils (we compile apiproto.c directly)
target_link_libraries(${target_name} PRIVATE aduc::c_utils)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_API_DEFAULT_FIFO_PATH="${ADUC_API_DEFAULT_FIFO_PATH}"
                           ADUC_DATA_FOLDER="${ADUC_DATA_FOLDER}"
                           ADUC_APIPROTO_NO_LOGGING=1)

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
