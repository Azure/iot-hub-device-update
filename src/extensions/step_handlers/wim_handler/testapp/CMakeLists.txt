
project (fmt)

include (agentRules)
compileasc99 ()
disablertti ()

add_executable (${PROJECT_NAME} "")
target_sources (${PROJECT_NAME} PRIVATE fmt.cpp)
target_include_directories (${PROJECT_NAME} PRIVATE inc)
target_link_libraries (${PROJECT_NAME} PRIVATE Wbemuuid.lib) # WBEM

