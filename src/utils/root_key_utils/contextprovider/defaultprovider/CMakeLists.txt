
set (target_name root_key_utils_default_context_provider)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)
target_sources(${target_name} PRIVATE src/default_rootkeyutil_contextprovider.c)
target_link_libraries (${target_name}
    PUBLIC aduc::c_utils
           aduc::root_key_utils_context_provider
    PRIVATE aduc::logging
            aduc::root_key_utils)


