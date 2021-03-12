project (adu_core_interface)

include (agentRules)

compileasc99 ()

add_library (
    ${PROJECT_NAME} STATIC
    src/adu_core_interface.c
    src/adu_core_json.c
    src/adu_core_export_helpers.c
    src/agent_workflow.c
    src/startup_msg_helper.c)

add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})
target_link_digital_twin_client (${PROJECT_NAME} PUBLIC)

find_package (Parson REQUIRED)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::adu_types
           aduc::c_utils
           aduc::communication_abstraction
           Parson::parson
    PRIVATE aduc::hash_utils
            aduc::jws_utils
            aduc::logging
            aduc::platform_layer
            aduc::pnp_helper)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_DEVICEPROPERTIES_MANUFACTURER="${ADUC_DEVICEPROPERTIES_MANUFACTURER}"
            ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}"
            ADUC_DEVICEPROPERTIES_MODEL="${ADUC_DEVICEPROPERTIES_MODEL}"
            ADUC_VERSION="${ADUC_VERSION}"
            ADUC_BUILDER_IDENTIFIER="${ADUC_BUILDER_IDENTIFIER}")

if (ADUC_PLATFORM_LAYER STREQUAL "simulator")
    target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_PLATFORM_SIMULATOR)
    if (ADUC_BUILD_UNIT_TESTS)
        # Unit tests for this library depend on the simulator being built, so
        # only build associated UT if we're building simulator.
        find_package (umock_c REQUIRED CONFIG)
        target_link_libraries (${PROJECT_NAME} PRIVATE umock_c)
        add_subdirectory (tests)
    endif ()
else ()
    find_package (deliveryoptimization_sdk CONFIG REQUIRED)
    target_link_libraries (${PROJECT_NAME} PRIVATE Microsoft::deliveryoptimization)
endif ()

if (ENABLE_ADU_TELEMETRY_REPORTING)
    target_compile_definitions (${PROJECT_NAME} PRIVATE ENABLE_ADU_TELEMETRY_REPORTING)
endif ()
