cmake_minimum_required (VERSION 3.5)

set (target_name platform_layer)

add_library (${target_name} INTERFACE)
add_library (aduc::${target_name} ALIAS ${target_name})

# Determine which platform layer needs to be built
# and add the appropriate subdirectory to the build.
if (ADUC_PLATFORM_LAYER STREQUAL "linux")
    add_subdirectory (linux_platform_layer)
    target_link_libraries (${target_name} INTERFACE aduc::linux_platform_layer)
elseif (ADUC_PLATFORM_LAYER STREQUAL "windows")
    add_subdirectory (windows_platform_layer)
    target_link_libraries (${target_name} INTERFACE aduc::windows_platform_layer)
else ()
    message (FATAL_ERROR "Invalid platform layer specified. ${ADUC_PLATFORM_LAYER}")
endif ()
