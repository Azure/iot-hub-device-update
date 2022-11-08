project (test_utils)

include (agentRules)
compileasc99 ()
disablertti ()

add_library (${PROJECT_NAME} STATIC "")
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)

target_sources (${PROJECT_NAME} PRIVATE src/auto_dir.cpp src/file_test_utils.cpp)

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::system_utils aduc::workflow_utils)
