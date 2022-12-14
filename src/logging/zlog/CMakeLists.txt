project (zlog)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC src/init.c src/zlog.c)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/inc
                                                   ${ADUC_LOGGING_INCLUDES})

if (_WIN32)
    # TODO(JeffMill): [PAL] dirent.h
    find_path (DIRENT_INCLUDE_DIRS "dirent.h")
    target_include_directories (${PROJECT_NAME} PRIVATE ${DIRENT_INCLUDE_DIRS})
endif ()

if (_WIN32)
    # TODO(JeffMill): [PAL] pthread
    find_package (PThreads4W REQUIRED)
    target_link_libraries (${PROJECT_NAME} PRIVATE PThreads4W::PThreads4W)
else ()
    find_package (Threads REQUIRED)
    target_link_libraries (${PROJECT_NAME} PRIVATE Threads::Threads)
endif ()

# _DEFAULT_SOURCE - Needed so DT_REG is defined in dirent.h
#                   see man page for readdir
#                   _BSD_SOURCE and _SVID_SOURCE are deprecated aliases for _DEFAULT_SOURCE.
#
# ADUC_USE_ZLOGGING - For zlog macros in logging.h
#
target_compile_definitions (${PROJECT_NAME} PRIVATE _DEFAULT_SOURCE ADUC_USE_ZLOGGING=1)
