# Core Azure Device Update SDK
# Shared utilities, result types, and common interfaces

cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED ADUC_CLIENT_SDK_BUILD)
    project(adu-core-sdk
            VERSION 1.0.0
            DESCRIPTION "Azure Device Update Core SDK"
            LANGUAGES C CXX)

    # Standalone build configuration
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

# Core SDK options
option(ADUC_CORE_SDK_BUILD_SHARED "Build core SDK as shared library" ON)
option(ADUC_CORE_SDK_BUILD_STATIC "Build core SDK as static library" OFF)
option(ADUC_CORE_SDK_BUILD_EXAMPLES "Build core SDK examples" OFF)
option(ADUC_CORE_SDK_BUILD_TESTS "Build core SDK tests" OFF)

# Source directories
set(ADUC_CORE_SDK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ADUC_CORE_SDK_INCLUDE_DIR ${ADUC_CORE_SDK_SOURCE_DIR}/inc)

# Include directories
include_directories(${ADUC_CORE_SDK_INCLUDE_DIR})

# Find dependencies
find_package(PkgConfig REQUIRED)

# Core SDK source files
set(ADUC_CORE_SDK_SOURCES
    src/aduc_result.c
    src/aduc_string_utils.c
    src/aduc_logging.c
    src/aduc_system_utils.c
    src/aduc_workflow_data.c
    src/aduc_config.c
    src/aduc_error_codes.c
    src/aduc_version.c
)

# Core SDK header files
set(ADUC_CORE_SDK_HEADERS
    inc/aduc/result.h
    inc/aduc/string_utils.h
    inc/aduc/logging.h
    inc/aduc/system_utils.h
    inc/aduc/workflow_data.h
    inc/aduc/config.h
    inc/aduc/error_codes.h
    inc/aduc/version.h
    inc/aduc/types.h
    inc/aduc/exports.h
)

# Build shared library
if(ADUC_CORE_SDK_BUILD_SHARED)
    add_library(aduc-core-sdk SHARED ${ADUC_CORE_SDK_SOURCES})

    target_include_directories(aduc-core-sdk
        PUBLIC
            $<BUILD_INTERFACE:${ADUC_CORE_SDK_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:include>
    )

    target_compile_definitions(aduc-core-sdk PRIVATE ADUC_SDK_EXPORTS)

    # Set library properties
    set_target_properties(aduc-core-sdk PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "${ADUC_CORE_SDK_HEADERS}"
    )

    # Install targets
    install(TARGETS aduc-core-sdk
        EXPORT aduc-core-sdk-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/aduc
    )
endif()

# Build static library
if(ADUC_CORE_SDK_BUILD_STATIC)
    add_library(aduc-core-sdk-static STATIC ${ADUC_CORE_SDK_SOURCES})

    target_include_directories(aduc-core-sdk-static
        PUBLIC
            $<BUILD_INTERFACE:${ADUC_CORE_SDK_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:include>
    )

    set_target_properties(aduc-core-sdk-static PROPERTIES
        OUTPUT_NAME aduc-core-sdk
        PUBLIC_HEADER "${ADUC_CORE_SDK_HEADERS}"
    )

    # Install targets
    install(TARGETS aduc-core-sdk-static
        EXPORT aduc-core-sdk-static-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/aduc
    )
endif()

# Create pkg-config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/aduc-core-sdk.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/aduc-core-sdk.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/aduc-core-sdk.pc"
        DESTINATION lib/pkgconfig
)

# Export targets
if(ADUC_CORE_SDK_BUILD_SHARED)
    install(EXPORT aduc-core-sdk-targets
        FILE aduc-core-sdk-targets.cmake
        NAMESPACE ADUC::
        DESTINATION lib/cmake/aduc-core-sdk
    )
endif()

if(ADUC_CORE_SDK_BUILD_STATIC)
    install(EXPORT aduc-core-sdk-static-targets
        FILE aduc-core-sdk-static-targets.cmake
        NAMESPACE ADUC::
        DESTINATION lib/cmake/aduc-core-sdk
    )
endif()

# Create CMake config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/aduc-core-sdk-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/aduc-core-sdk-config.cmake"
    INSTALL_DESTINATION lib/cmake/aduc-core-sdk
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/aduc-core-sdk-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/aduc-core-sdk-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/aduc-core-sdk-config-version.cmake"
    DESTINATION lib/cmake/aduc-core-sdk
)

# Add subdirectories
if(ADUC_CORE_SDK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(ADUC_CORE_SDK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Generate documentation
option(ADUC_CORE_SDK_BUILD_DOCS "Build core SDK documentation" OFF)
if(ADUC_CORE_SDK_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()
