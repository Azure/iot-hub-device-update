
set (target_name device_provisioning_client)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_link_aziotsharedutil (${target_name} PUBLIC)

target_include_directories (${target_name} PUBLIC inc)
target_sources (${target_name} PRIVATE src/device_provisioning_client.c)

target_link_libraries (${target_name} PRIVATE aduc::logging)
