set (target_name shared_lib)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC "")
add_library (aduc::${target_name} ALIAS ${target_name})

# Turn -fPIC on, in order to use this library in a shared library.
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_sources (${target_name} PRIVATE src/shared_lib.cpp inc/aduc/plugin_call_helper.hpp)

target_link_libraries (${target_name} PRIVATE aduc::hash_utils ${CMAKE_DL_LIBS})
