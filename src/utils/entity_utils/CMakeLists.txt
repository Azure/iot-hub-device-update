set (target_name entity_utils)

add_library (${target_name} INTERFACE)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} INTERFACE inc)
target_link_aziotsharedutil (${target_name} INTERFACE)
