cmake_minimum_required (VERSION 3.5)

if (ADUC_WARNINGS_AS_ERRORS)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif ()

set (COMPILER_HARDENING_FLAGS
     "-fPIE -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Werror=format-security")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -z relro -z now")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_HARDENING_FLAGS}")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_HARDENING_FLAGS} -Wall")

add_definitions (-DADUC_LOG_FOLDER="${ADUC_LOG_FOLDER}")

set (ADUC_EXPORT_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/inc)
set (ADU_SHELL_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/adu-shell/inc)

add_subdirectory (agent)
add_subdirectory (adu-shell)
add_subdirectory (adu_types)
add_subdirectory (communication_abstraction)
add_subdirectory (content_handlers)

if (ADUC_BUILD_DOCUMENTATION)
    add_subdirectory (docs)
endif ()

add_subdirectory (logging)
add_subdirectory (platform_layers)
add_subdirectory (utils)
