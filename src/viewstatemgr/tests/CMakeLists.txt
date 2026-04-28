set (target_name viewstatemgr_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE viewstatemgr_unit_tests.cpp)

target_link_libraries (
    ${target_name}
    PRIVATE Catch2::Catch2WithMain
            aduc::auto_utils
            aduc::viewstatemgr)

if (NOT WIN32)
    target_link_libraries (${target_name} PRIVATE aduc::sdk)
else ()
    # On Windows, SDK library is not built (POSIX-only FIFO IPC).
    # Only the header is needed for ADUC_ServiceStatus enum.
    target_include_directories (${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/src/sdk/inc)
endif ()

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${target_name})
else ()
    add_test (NAME ${target_name} COMMAND ${target_name})
endif ()