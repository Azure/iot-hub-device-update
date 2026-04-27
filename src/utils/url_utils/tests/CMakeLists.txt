set (target_name url_utils_unit_test)

project (url_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE https_proxy_utils_ut.cpp url_utils_ut.cpp)

target_link_libraries (${target_name} PRIVATE aduc::url_utils Catch2::Catch2WithMain)
target_link_libraries (${target_name} PRIVATE libaducpal)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${target_name})
else ()
    add_test (NAME ${target_name} COMMAND ${target_name})
endif ()