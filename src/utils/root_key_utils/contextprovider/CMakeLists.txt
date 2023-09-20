set (target_name root_key_utils_context_provider)

include (agentRules)
compileasc99 ()

add_library (${target_name} INTERFACE)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} INTERFACE inc)
target_link_libraries (${target_name} INTERFACE aduc::c_utils aduc::root_key_utils)

add_subdirectory(defaultprovider)
