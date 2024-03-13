set (target_name zlog)

include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC src/init.c src/zlog.c)

target_sources (${target_name} PRIVATE src/init.c src/zlog.c)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
                                                  ${ADUC_LOGGING_INCLUDES})

if (WIN32)
    find_package (PThreads4W REQUIRED)
    target_link_libraries (${target_name} PRIVATE PThreads4W::PThreads4W aduc::system_utils)
else ()
    find_package (Threads REQUIRED)
    target_link_libraries (${target_name} PRIVATE Threads::Threads aduc::system_utils)
endif ()

target_link_libraries (${target_name} PRIVATE libaducpal aduc::system_utils)

# _DEFAULT_SOURCE - Needed so DT_REG is defined in dirent.h
#                   see man page for readdir
#                   _BSD_SOURCE and _SVID_SOURCE are deprecated aliases for _DEFAULT_SOURCE.
#
# ADUC_USE_ZLOGGING - For zlog macros in logging.h
#
target_compile_definitions (${target_name} PRIVATE _DEFAULT_SOURCE ADUC_USE_ZLOGGING=1)
