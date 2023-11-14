set (target_name agentinfo_utils)

add_library (${target_name} STATIC src/adu_agentinfo_utils.c)

add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

find_package (Parson REQUIRED)
target_link_libraries (${target_name}
    PUBLIC aduc::retry_utils Parson::parson
    PRIVATE aduc::agent_state_store
            aduc::logging
            aduc::c_utils)

add_subdirectory (tests)
