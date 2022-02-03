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
add_definitions (-DADUC_DOWNLOADS_FOLDER="${ADUC_DOWNLOADS_FOLDER}")

set (ADUC_TYPES_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/adu_types/inc)
set (ADUC_EXPORT_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/inc ${CMAKE_CURRENT_SOURCE_DIR}/adu_types/inc)
set (ADU_SHELL_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/adu-shell/inc)
set (ADU_EXTENSION_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/extensions/inc
                            ${CMAKE_CURRENT_SOURCE_DIR}/adu_types/inc)

add_subdirectory (adu-shell)
add_subdirectory (adu_types)
add_subdirectory (adu_workflow)
add_subdirectory (communication_abstraction)
add_subdirectory (content_handlers)
add_subdirectory (diagnostics_component)

if (ADUC_BUILD_DOCUMENTATION)
    add_subdirectory (docs)
endif ()

add_subdirectory (logging)
add_subdirectory (platform_layers)
add_subdirectory (utils)
add_subdirectory (extensions)
add_subdirectory (agent)
