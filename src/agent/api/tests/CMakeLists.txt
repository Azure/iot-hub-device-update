set (target_name apisvc_unit_tests)

include (agentRules)
# Do not specify C99 for test code because Catch2 uses C++ features.
# compile_as_c99 () includes set (CMAKE_C_STARTDARD 99) which may cause
# issues with C++ 17 features in come build configurations.

# compileasc99 ()

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE apisvc_unit_tests.cpp)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_API_DEFAULT_FIFO_PATH="${ADUC_API_DEFAULT_FIFO_PATH}"
                           ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
                           ADUC_FILE_USER="${ADUC_FILE_USER}")

target_link_libraries (
    ${target_name}
    PRIVATE Catch2::Catch2WithMain
            aduc::apiproto_utils
            aduc::apisvc
            aduc::auto_utils
            aduc::config_utils
            aduc::logging
            aduc::sdk
            aduc::viewstatemgr)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})

# Add container-specific tests as an optional target
set (container_target_name apisvc_container_unit_tests)

add_executable (${container_target_name})
target_sources (${container_target_name} PRIVATE apisvc_container_unit_tests.cpp)

target_compile_definitions (
    ${container_target_name} PRIVATE ADUC_API_DEFAULT_FIFO_PATH="${ADUC_API_DEFAULT_FIFO_PATH}"
                                     ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
                                     ADUC_FILE_USER="${ADUC_FILE_USER}")

target_link_libraries (
    ${container_target_name}
    PRIVATE Catch2::Catch2WithMain
            aduc::apiproto_utils
            aduc::apisvc
            aduc::auto_utils
            aduc::config_utils
            aduc::logging
            aduc::sdk
            aduc::viewstatemgr)

# Register container tests but mark them as potentially long-running
catch_discover_tests (${container_target_name}
    PROPERTIES
        LABELS "container;slow"
        TIMEOUT 60)
