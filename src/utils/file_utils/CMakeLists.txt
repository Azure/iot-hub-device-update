set (target_name file_utils)

include (agentRules)

compileasc99 ()
disablertti ()

add_library (${target_name} STATIC "")
add_library (aduc::${target_name} ALIAS ${target_name})

# Turn -fPIC on, in order to use this library in another shared library.
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc)

target_sources (${target_name} PRIVATE src/auto_opendir.cpp src/file_utils.cpp)

target_link_libraries (${target_name} PRIVATE aduc::system_utils)

target_link_libraries (${target_name} PRIVATE libaducpal)
