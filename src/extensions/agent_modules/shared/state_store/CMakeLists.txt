
set (target_name agent_state_store)

include (agentRules)

add_library (${target_name} STATIC "")
add_library (aduc::${target_name} ALIAS ${target_name})

find_package (Parson REQUIRED)

# Turn -fPIC on, in order to use this library in a shared library.
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC ./inc)

target_sources (${target_name} PRIVATE src/agent_state_store.c)

target_link_libraries (${target_name}
    PUBLIC aduc::c_utils Parson::parson
    PRIVATE aduc::logging)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
