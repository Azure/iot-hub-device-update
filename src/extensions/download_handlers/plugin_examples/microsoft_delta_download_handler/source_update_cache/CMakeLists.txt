cmake_minimum_required (VERSION 3.5)

set (target_name source_update_cache)

include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC "")
add_library (aduc::${target_name} ALIAS ${target_name})

# Turn -fPIC on, in order to use this library in another shared library.
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_sources (${target_name} PRIVATE src/source_update_cache.c src/source_update_cache_utils.cpp
                                       src/source_update_cache_utils.c)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types aduc::c_utils
    PRIVATE aduc::file_utils aduc::parser_utils aduc::path_utils aduc::permission_utils
            aduc::system_utils aduc::workflow_utils)

target_compile_definitions (
    ${target_name}
    PRIVATE
        ADUC_DELTA_DOWNLOAD_HANDLER_SOURCE_UPDATE_CACHE_DIR="${ADUC_DELTA_DOWNLOAD_HANDLER_SOURCE_UPDATE_CACHE_DIR}"
)

if (ADUC_DELTA_DOWNLOAD_HANDLER_SOURCE_UPDATE_CACHE_COMMIT_STRATEGY STREQUAL "TWO_PHASE_COMMIT")
    target_compile_definitions (${target_name} PRIVATE TWO_PHASE_COMMIT="1")
endif ()

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
