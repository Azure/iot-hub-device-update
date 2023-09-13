set (target_name rootkey_store)

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources(${target_name} PRIVATE src/rootkey_store.hpp src/rootkey_store.cpp)
target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})
target_link_libraries (${target_name} PUBLIC aduc::c_utils PRIVATE aduc::logging aduc::rootkeypackage_utils)

# if (ADUC_BUILD_UNIT_TESTS)
#     add_subdirectory (tests)
# endif ()
