# Step Handler SDK
# Provides APIs for creating custom content handlers for Azure Device Update

cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED ADUC_CLIENT_SDK_BUILD)
    project(adu-step-handler-sdk
            VERSION 1.0.0
            DESCRIPTION "Azure Device Update Step Handler SDK"
            LANGUAGES C CXX)
    
    # Standalone build configuration
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

# Step Handler SDK options
option(ADUC_STEP_HANDLER_SDK_BUILD_SHARED "Build step handler SDK as shared library" ON)
option(ADUC_STEP_HANDLER_SDK_BUILD_STATIC "Build step handler SDK as static library" OFF)
option(ADUC_STEP_HANDLER_SDK_BUILD_EXAMPLES "Build step handler SDK examples" OFF)
option(ADUC_STEP_HANDLER_SDK_BUILD_TESTS "Build step handler SDK tests" OFF)
option(ADUC_STEP_HANDLER_SDK_COPY_EXISTING "Copy existing handlers as examples" OFF)

# Find dependencies
find_package(PkgConfig REQUIRED)

# Try to find Core SDK via cmake first
list(APPEND CMAKE_PREFIX_PATH /usr/local)
find_package(aduc-core-sdk QUIET)

if(NOT aduc-core-sdk_FOUND)
    # Fallback to pkg-config
    set(ENV{PKG_CONFIG_PATH} "/usr/local/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    pkg_check_modules(ADUC_CORE_SDK REQUIRED adu-core-sdk)
endif()

# Source directories
set(ADUC_STEP_HANDLER_SDK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ADUC_STEP_HANDLER_SDK_INCLUDE_DIR ${ADUC_STEP_HANDLER_SDK_SOURCE_DIR}/inc)

# Include directories
include_directories(${ADUC_STEP_HANDLER_SDK_INCLUDE_DIR})

# Step Handler SDK source files
set(ADUC_STEP_HANDLER_SDK_SOURCES
    src/content_handler.c
    src/step_handler_interface.c
    src/workflow_data_utils.c
    src/content_downloader_helper.c
    src/manifest_handler_helper.c
    src/handler_registry.c
)

# Step Handler SDK header files
set(ADUC_STEP_HANDLER_SDK_HEADERS
    inc/aduc/content_handler.h
    inc/aduc/step_handler_interface.h
    inc/aduc/workflow_data_utils.h
    inc/aduc/content_downloader_helper.h
    inc/aduc/manifest_handler_helper.h
    inc/aduc/handler_registry.h
    inc/aduc/step_handler_sdk.h
)

# Build shared library
if(ADUC_STEP_HANDLER_SDK_BUILD_SHARED)
    add_library(adu-step-handler-sdk SHARED ${ADUC_STEP_HANDLER_SDK_SOURCES})
    
    target_include_directories(adu-step-handler-sdk 
        PUBLIC 
            $<BUILD_INTERFACE:${ADUC_STEP_HANDLER_SDK_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${ADUC_CORE_SDK_INCLUDE_DIRS}
    )
    
    target_link_libraries(adu-step-handler-sdk 
        PUBLIC 
            ${ADUC_CORE_SDK_LIBRARIES}
    )
    
    target_compile_definitions(adu-step-handler-sdk PRIVATE ADUC_SDK_EXPORTS)
    
    # Set library properties
    set_target_properties(adu-step-handler-sdk PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "${ADUC_STEP_HANDLER_SDK_HEADERS}"
    )
    
    # Install targets
    install(TARGETS adu-step-handler-sdk
        EXPORT adu-step-handler-sdk-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/aduc
    )
endif()

# Build static library
if(ADUC_STEP_HANDLER_SDK_BUILD_STATIC)
    add_library(adu-step-handler-sdk-static STATIC ${ADUC_STEP_HANDLER_SDK_SOURCES})
    
    target_include_directories(adu-step-handler-sdk-static 
        PUBLIC 
            $<BUILD_INTERFACE:${ADUC_STEP_HANDLER_SDK_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${ADUC_CORE_SDK_INCLUDE_DIRS}
    )
    
    target_link_libraries(adu-step-handler-sdk-static 
        PUBLIC 
            ${ADUC_CORE_SDK_LIBRARIES}
    )
    
    set_target_properties(adu-step-handler-sdk-static PROPERTIES
        OUTPUT_NAME adu-step-handler-sdk
        PUBLIC_HEADER "${ADUC_STEP_HANDLER_SDK_HEADERS}"
    )
    
    # Install targets
    install(TARGETS adu-step-handler-sdk-static
        EXPORT adu-step-handler-sdk-static-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/aduc
    )
endif()

# Create pkg-config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/adu-step-handler-sdk.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/adu-step-handler-sdk.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/adu-step-handler-sdk.pc"
        DESTINATION lib/pkgconfig
)

# Export targets and create config files
if(ADUC_STEP_HANDLER_SDK_BUILD_SHARED)
    install(EXPORT adu-step-handler-sdk-targets
        FILE adu-step-handler-sdk-targets.cmake
        NAMESPACE ADUC::
        DESTINATION lib/cmake/adu-step-handler-sdk
    )
endif()

if(ADUC_STEP_HANDLER_SDK_BUILD_STATIC)
    install(EXPORT adu-step-handler-sdk-static-targets
        FILE adu-step-handler-sdk-static-targets.cmake
        NAMESPACE ADUC::
        DESTINATION lib/cmake/adu-step-handler-sdk
    )
endif()

# Create CMake config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/adu-step-handler-sdk-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/adu-step-handler-sdk-config.cmake"
    INSTALL_DESTINATION lib/cmake/adu-step-handler-sdk
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/adu-step-handler-sdk-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/adu-step-handler-sdk-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/adu-step-handler-sdk-config-version.cmake"
    DESTINATION lib/cmake/adu-step-handler-sdk
)

# Add subdirectories
if(ADUC_STEP_HANDLER_SDK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(ADUC_STEP_HANDLER_SDK_COPY_EXISTING)
    add_subdirectory(existing)
endif()

if(ADUC_STEP_HANDLER_SDK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Generate documentation
option(ADUC_STEP_HANDLER_SDK_BUILD_DOCS "Build step handler SDK documentation" OFF)
if(ADUC_STEP_HANDLER_SDK_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()