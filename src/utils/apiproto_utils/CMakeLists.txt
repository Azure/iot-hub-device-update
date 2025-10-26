
set (target_name apiproto_utils)

include (agentRules)
compileasc99 ()
disablertti ()

add_library (${target_name} OBJECT)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources (${target_name} PRIVATE src/apiproto.c)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aduc::logging)

# Enable coverage for this library when building tests
if (ADUC_BUILD_UNIT_TESTS AND ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE --coverage -g -O0)
    endif()
endif()

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
