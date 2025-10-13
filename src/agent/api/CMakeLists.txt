set (target_name apisvc)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources(${target_name} PRIVATE src/apisvc.c)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_API_DEFAULT_FIFO_PATH="${ADUC_API_DEFAULT_FIFO_PATH}"
                           ADUC_FILE_GROUP="${ADUC_FILE_GROUP}" ADUC_FILE_USER="${ADUC_FILE_USER}")
if (WIN32)
    find_package (PThreads4W REQUIRED)
    target_link_libraries (${target_name} PRIVATE PThreads4W::PThreads4W)
endif ()

target_link_libraries (${target_name} PRIVATE aduc::viewstatemgr aduc::config_utils aduc::apiproto_utils)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_libraries (${target_name} PRIVATE aduc::logging aduc::permission_utils libaducpal)

# Add tests subdirectory if building tests
if (BUILD_TESTING)
    add_subdirectory (tests)
endif ()
