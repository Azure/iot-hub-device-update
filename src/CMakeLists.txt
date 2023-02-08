cmake_minimum_required (VERSION 3.5)

if (ADUC_TRACE_TARGET_DEPS)
    set_property (GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
endif ()

if (ADUC_WARNINGS_AS_ERRORS)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif ()

if (NOT WIN32)
    # Avoids readdir returning errno 75 EOVERFLOW when running
    # 32-bit ARM docker/qemu on top of 64-bit host by using this
    # glibc-specific macro. See https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
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
add_subdirectory (communication_managers)
add_subdirectory (diagnostics_component)

if (ADUC_BUILD_DOCUMENTATION)
    add_subdirectory (docs)
endif ()

add_subdirectory (logging)
add_subdirectory (platform_layers)
add_subdirectory (utils)
add_subdirectory (extensions)
add_subdirectory (agent)
add_subdirectory (agent_orchestration)
