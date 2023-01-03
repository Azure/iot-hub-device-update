project (file_utils)

include (agentRules)

compileasc99 ()
disablertti ()

add_library (${PROJECT_NAME} STATIC "")
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

if (WIN32)
    # TODO(JeffMill): [PAL] dirent.h
    find_path (DIRENT_INCLUDE_DIRS "dirent.h")
    target_include_directories (${PROJECT_NAME} PRIVATE ${DIRENT_INCLUDE_DIRS})
endif ()

# Turn -fPIC on, in order to use this library in another shared library.
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${PROJECT_NAME} PUBLIC inc)

target_sources (${PROJECT_NAME} PRIVATE src/auto_opendir.cpp src/file_utils.cpp)

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::system_utils)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)
