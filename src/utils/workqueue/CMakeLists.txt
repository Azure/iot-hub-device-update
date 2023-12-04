set (target_name workqueue)

include (agentRules)
compileasc99 ()
disablertti ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})
target_sources (${target_name} PRIVATE src/workqueue.hpp src/workqueue.cpp src/workqueue_worker.cpp)

# Turn -fPIC on, in order to use this library in another shared library.
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries (${target_name} PUBLIC aduc::c_utils)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
