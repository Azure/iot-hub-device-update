project (azure_iot_aduc_agent)
set (target_name download_utils)

include (agentRules)
compileasc99 ()
disablertti ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (${target_name} PRIVATE src/download_utils.cpp)

target_include_directories (${target_name} PUBLIC inc)

target_link_libraries (${target_name} PUBLIC aduc::adu_types)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
