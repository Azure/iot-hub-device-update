set (target_name test_utils)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (OpenSSL REQUIRED)

add_library (${target_name} STATIC "")
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

target_sources (${target_name} PRIVATE src/auto_dir.cpp src/file_test_utils.cpp src/crypto_test_utils.cpp)

target_link_libraries (${target_name} PRIVATE aduc::system_utils aduc::workflow_utils OpenSSL::Crypto)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
