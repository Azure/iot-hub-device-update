cmake_minimum_required(VERSION 3.5)

# Azure Device Update Extension SDK Suite
project(adu-extension-sdk-suite VERSION 1.0.0)

# SDK Suite Options
option(ADUC_BUILD_EXTENSION_SDK_SUITE "Build the complete ADU Extension SDK suite" OFF)
option(ADUC_BUILD_STEP_HANDLER_SDK "Build the Step Handler SDK" OFF)
option(ADUC_BUILD_MANIFEST_HANDLER_SDK "Build the Update Manifest Handler SDK" OFF)
option(ADUC_BUILD_COMMUNICATION_SDK "Build the Communication Extension SDK" OFF)
option(ADUC_BUILD_CONTENT_DOWNLOADER_SDK "Build the Content Downloader SDK" OFF)
option(ADUC_BUILD_DOWNLOAD_HANDLER_SDK "Build the Download Handler SDK" OFF)
option(ADUC_BUILD_LOGGING_SDK "Build the Logging Extension SDK" OFF)
option(ADUC_BUILD_CONFIGURATION_SDK "Build the Configuration Extension SDK" OFF)

# SDK-specific options
option(ADUC_SDK_BUILD_EXAMPLES "Build SDK example extensions" OFF)
option(ADUC_SDK_BUILD_TOOLS "Build SDK development tools" OFF)
option(ADUC_SDK_BUILD_STATIC "Build static libraries for SDKs" OFF)
option(ADUC_SDK_COPY_EXISTING_EXTENSIONS "Copy existing in-tree extensions as examples" ON)

# Auto-enable dependencies when building the full suite
if(ADUC_BUILD_EXTENSION_SDK_SUITE)
    set(ADUC_BUILD_STEP_HANDLER_SDK ON)
    set(ADUC_BUILD_MANIFEST_HANDLER_SDK ON)
    set(ADUC_BUILD_COMMUNICATION_SDK ON)
    set(ADUC_BUILD_CONTENT_DOWNLOADER_SDK ON)
    set(ADUC_BUILD_DOWNLOAD_HANDLER_SDK ON)
    set(ADUC_BUILD_LOGGING_SDK ON)
    set(ADUC_BUILD_CONFIGURATION_SDK ON)
    set(ADUC_SDK_BUILD_EXAMPLES ON)
    set(ADUC_SDK_BUILD_TOOLS ON)
endif()

# Check if any SDK component is enabled
if(ADUC_BUILD_EXTENSION_SDK_SUITE OR
   ADUC_BUILD_STEP_HANDLER_SDK OR
   ADUC_BUILD_MANIFEST_HANDLER_SDK OR
   ADUC_BUILD_COMMUNICATION_SDK OR
   ADUC_BUILD_CONTENT_DOWNLOADER_SDK OR
   ADUC_BUILD_DOWNLOAD_HANDLER_SDK OR
   ADUC_BUILD_LOGGING_SDK OR
   ADUC_BUILD_CONFIGURATION_SDK)

    message(STATUS "Building ADU Extension SDK components...")

    # Add core SDK (always built when any SDK is enabled)
    add_subdirectory(core)

    # Add individual SDK components as requested
    if(ADUC_BUILD_STEP_HANDLER_SDK OR ADUC_BUILD_EXTENSION_SDK_SUITE)
        message(STATUS "  - Step Handler SDK")
        add_subdirectory(step-handlers)
    endif()

    if(ADUC_BUILD_MANIFEST_HANDLER_SDK OR ADUC_BUILD_EXTENSION_SDK_SUITE)
        message(STATUS "  - Update Manifest Handler SDK")
        add_subdirectory(update-manifest-handlers)
    endif()

    if(ADUC_BUILD_COMMUNICATION_SDK OR ADUC_BUILD_EXTENSION_SDK_SUITE)
        message(STATUS "  - Communication Extension SDK")
        add_subdirectory(communication)
    endif()

    if(ADUC_BUILD_CONTENT_DOWNLOADER_SDK OR ADUC_BUILD_EXTENSION_SDK_SUITE)
        message(STATUS "  - Content Downloader SDK")
        add_subdirectory(content-downloaders)
    endif()

    if(ADUC_BUILD_DOWNLOAD_HANDLER_SDK OR ADUC_BUILD_EXTENSION_SDK_SUITE)
        message(STATUS "  - Download Handler SDK")
        add_subdirectory(download-handlers)
    endif()

    if(ADUC_BUILD_LOGGING_SDK OR ADUC_BUILD_EXTENSION_SDK_SUITE)
        message(STATUS "  - Logging Extension SDK")
        add_subdirectory(logging)
    endif()

    if(ADUC_BUILD_CONFIGURATION_SDK OR ADUC_BUILD_EXTENSION_SDK_SUITE)
        message(STATUS "  - Configuration Extension SDK")
        add_subdirectory(configuration)
    endif()

    # Add development tools if requested
    if(ADUC_SDK_BUILD_TOOLS)
        message(STATUS "  - SDK Development Tools")
        add_subdirectory(tools)
    endif()

    # Add SDK suite package if building the full suite
    if(ADUC_BUILD_EXTENSION_SDK_SUITE)
        message(STATUS "  - SDK Suite Package")
        add_subdirectory(suite)
    endif()

else()
    message(STATUS "ADU Extension SDK components disabled")
endif()

# Create top-level SDK documentation target
if(ADUC_BUILD_EXTENSION_SDK_SUITE OR
   ADUC_BUILD_STEP_HANDLER_SDK OR
   ADUC_BUILD_MANIFEST_HANDLER_SDK OR
   ADUC_BUILD_COMMUNICATION_SDK OR
   ADUC_BUILD_CONTENT_DOWNLOADER_SDK OR
   ADUC_BUILD_DOWNLOAD_HANDLER_SDK OR
   ADUC_BUILD_LOGGING_SDK OR
   ADUC_BUILD_CONFIGURATION_SDK)

    # Custom target for documentation
    add_custom_target(adu-sdk-docs
        COMMAND ${CMAKE_COMMAND} -E echo "Building ADU SDK documentation..."
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generate ADU Extension SDK documentation"
    )

endif()
