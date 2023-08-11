set (target_name shutdown_service)

include (agentRules)
compileasc99 ()
disablertti ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (${target_name} PRIVATE src/shutdown_service.c)

target_include_directories (${target_name} PUBLIC inc)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (${target_name} PUBLIC aduc::c_utils)
