project (microsoft_delta_download_handler_utils)

include (agentRules)
compileasc99 ()

add_library (${PROJECT_NAME} STATIC "")
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Turn -fPIC on, in order to use this library in a shared library.
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_sources (${PROJECT_NAME} PRIVATE src/microsoft_delta_download_handler_utils.c
                                        src/microsoft_delta_download_handler_utils.cpp)

find_package (azure_c_shared_utility REQUIRED)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::adu_types aziotsharedutil
    PRIVATE aduc::c_utils
            aduc::extension_manager
            aduc::logging
            aduc::shared_lib
            aduc::source_update_cache
            aduc::workflow_utils)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
