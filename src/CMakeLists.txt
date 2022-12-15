cmake_minimum_required (VERSION 3.5)

if (ADUC_TRACE_TARGET_DEPS)
    set_property (GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
endif ()

if (ADUC_WARNINGS_AS_ERRORS)
    if (MSVC)
        # TODO(JeffMill): Use /W4 (noisy) instead?
        add_compile_options (/W3 /WX)

        # TODO(JeffMill): allow strcpy, strcat, fopen, etc.
        add_definitions (-D_CRT_SECURE_NO_WARNINGS)

        # TODO(JeffMill): Avoid C4996: POSIX name for this item is deprecated.
        add_definitions (-D_CRT_NONSTDC_NO_WARNINGS)
    else ()
        add_compile_options (-Werror)
    endif ()
endif ()

if (MSVC)
    # TODO(JeffMill): Determine what the equivalent on MSVC is.
else ()
    set (COMPILER_HARDENING_FLAGS
         "-fPIE -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Werror=format-security")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -z relro -z now")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_HARDENING_FLAGS}")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_HARDENING_FLAGS} -Wall")
endif ()

add_definitions (-DADUC_LOG_FOLDER="${ADUC_LOG_FOLDER}")
add_definitions (-DADUC_DOWNLOADS_FOLDER="${ADUC_DOWNLOADS_FOLDER}")

set (ADUC_TYPES_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/adu_types/inc)
set (ADUC_EXPORT_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/inc ${CMAKE_CURRENT_SOURCE_DIR}/adu_types/inc)
set (ADU_SHELL_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/adu-shell/inc)
set (ADU_EXTENSION_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/extensions/inc
                            ${CMAKE_CURRENT_SOURCE_DIR}/adu_types/inc)

add_subdirectory (libaducpal)
add_subdirectory (adu-shell)
add_subdirectory (adu_types)
add_subdirectory (adu_workflow)
add_subdirectory (communication_abstraction)
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
