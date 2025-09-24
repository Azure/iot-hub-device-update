
set (target_name viewstatemgr)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Parson REQUIRED)

add_library (${target_name} OBJECT)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (${target_name} PRIVATE src/viewstatemgr.c)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aduc::logging Parson::parson)

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_DATASTORE_FILE_PATH="${ADUC_DATASTORE_FILE_PATH}")

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
