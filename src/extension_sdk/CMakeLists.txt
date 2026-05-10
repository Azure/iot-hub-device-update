cmake_minimum_required(VERSION 3.14)
project(aduc_extension_sdk C)

# ─── TOML parser (third-party, single file) ──────────────────────────────────
add_library(tomlc99 STATIC
    third_party/tomlc99/toml.c
)
target_include_directories(tomlc99 PUBLIC
    third_party/tomlc99
)
# Suppress warnings in third-party code
if(MSVC)
    target_compile_options(tomlc99 PRIVATE /w)
else()
    target_compile_options(tomlc99 PRIVATE -w)
endif()

# ─── Extension SDK library ───────────────────────────────────────────────────
find_package(Parson REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(aduc_extension_sdk STATIC
    src/adu_direct_mtls.c
    src/comm_dispatcher.c
    src/config_reader.c
    src/download_manager.c
    src/download_service.c
    src/extension_loader.c
    src/log_callback.c
    src/log_writer.c
    src/manifest_parser.c
    src/process_supervisor.c
    src/step_result.c
    src/workflow_engine.c
    src/dag_engine.c
    src/workflow_persist.c
    src/process_identity.c
    src/process_context.c
    src/process_ipc_report.c
    src/child_reporter.c
    src/component_registry.c
    src/component_targeting.c
    src/component.c
    src/content_pipeline.c
    src/extension_signing.c
    src/secure_storage.c
    src/cert_lifecycle.c
    src/log_manifest.c
)

target_include_directories(aduc_extension_sdk PUBLIC
    inc
)

target_link_libraries(aduc_extension_sdk
    PUBLIC tomlc99 Parson::parson
    PRIVATE CURL::libcurl
    PRIVATE OpenSSL::Crypto
)

if(WIN32)
    target_compile_definitions(aduc_extension_sdk PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else()
    target_link_libraries(aduc_extension_sdk
        PRIVATE ${CMAKE_DL_LIBS}   # dlopen/dlsym on Linux
        PRIVATE pthread
    )
endif()

# Add alias for consistent naming
add_library(aduc::extension_sdk ALIAS aduc_extension_sdk)

# ─── Tests ────────────────────────────────────────────────────────────────────
if(ADUC_BUILD_UNIT_TESTS)
    add_subdirectory(tests)
endif()
