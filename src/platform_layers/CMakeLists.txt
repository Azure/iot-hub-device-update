cmake_minimum_required (VERSION 3.5)

project (platform_layer)

add_library (${PROJECT_NAME} INTERFACE)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Determine which platform layer needs to be built
# and add the appropriate subdirectory to the build.
if (ADUC_PLATFORM_LAYER STREQUAL "linux")
    add_subdirectory (linux_platform_layer)
    target_link_libraries (${PROJECT_NAME} INTERFACE aduc::linux_platform_layer)
else ()
    message (FATAL_ERROR "Invalid platform layer specified. ${ADUC_PLATFORM_LAYER}")
endif ()
