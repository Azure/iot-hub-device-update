cmake_minimum_required (VERSION 3.5)

set (target_name diagnostics_devicename)

add_library (${target_name} STATIC src/diagnostics_devicename.c)
add_library (diagnostics_component::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

target_link_aziotsharedutil (${target_name} PRIVATE)

# TODO(JeffMill): Use target_link_umock_c()?
if (_WIN32)
    # TODO(JeffMill): [VCPKG] fatal  error C1083: Cannot open include file: 'umock_c/umock_c_prod.h': No such file or directory
    target_include_directories (${target_name} PRIVATE ${UMOCK_C_INC_FOLDER})
endif ()

target_link_libraries (${target_name} PUBLIC aduc::c_utils)
