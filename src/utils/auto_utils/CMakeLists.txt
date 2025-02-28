
set (target_name auto_utils)

add_library (${target_name} INTERFACE)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} INTERFACE inc)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)
