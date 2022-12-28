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

find_package (Threads REQUIRED)

target_link_libraries (${PROJECT_NAME} PRIVATE Threads::Threads aduc::system_utils)

# _DEAFULT_SOURCE - Needed so DT_REG is defined in dirent.h
#                   see man page for readdir
#                   _BSD_SOURCE and _SVID_SOURCE are deprecated aliases for _DEFAULT_SOURCE.
#
# ADUC_USE_ZLOGGING - For zlog macros in logging.h
#
target_compile_definitions (${PROJECT_NAME} PRIVATE _DEFAULT_SOURCE ADUC_USE_ZLOGGING=1)
