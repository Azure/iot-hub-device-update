cmake_minimum_required (VERSION 3.5)

include (agentRules)

compileasc99 ()

if (ADUC_TRACE_TARGET_DEPS)
    set_property (GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
endif ()

if (ADUC_USE_TEST_ROOT_KEYS)
    set (EMBED_TEST_ROOT_KEYS 1)
else ()
    set (EMBED_TEST_ROOT_KEYS 0)
endif ()

if (ADUC_WARNINGS_AS_ERRORS)
    if (MSVC)
        # Warnings as errors.
        add_compile_options (/WX)
    else ()
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    endif ()
endif ()

if (NOT WIN32)
    # Avoids readdir returning errno 75 EOVERFLOW when running
    # 32-bit ARM docker/qemu on top of 64-bit host by using this
    # glibc-specific macro. See https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
endif ()

if (MSVC)
    # Warning level 4
    add_compile_options (/W4 /guard:cf /Qspectre)

    if (NOT ADUC_BUILD_UNIT_TESTS)
        # Treat Conversion Warnings as Errors
        add_compile_options (/W4242)
    endif()

    # allow strcpy, strcat, fopen, etc.
    add_definitions (-D_CRT_SECURE_NO_WARNINGS)
    # Avoid C4996: POSIX name for this item is deprecated.
    add_definitions (-D_CRT_NONSTDC_NO_WARNINGS)

    # Avoid "warning LNK4098: defaultlib 'LIBCMT' conflicts with use of other libs; use /NODEFAULTLIB:library"
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:msvcrt /guard:cf")
    else ()
        set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt /guard:cf")
    endif ()
else ()

    set (COMPILER_HARDENING_FLAGS
         "-fPIE -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Werror=format-security")

    if (NOT ADUC_BUILD_UNIT_TESTS)
        set (COMPILER_HARDENING_FLAGS "${COMPILER_HARDENING_FLAGS}  -Wconversion")
    endif ()

    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -z relro -z now")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_HARDENING_FLAGS}")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_HARDENING_FLAGS} -Wall")
endif ()

add_definitions (-DADUC_LOG_FOLDER="${ADUC_LOG_FOLDER}")

set (ADUC_TYPES_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/adu_types/inc)
set (ADUC_EXPORT_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/inc ${CMAKE_CURRENT_SOURCE_DIR}/adu_types/inc)
set (ADU_SHELL_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/adu-shell/inc)
set (ADU_EXTENSION_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/extensions/inc
                            ${CMAKE_CURRENT_SOURCE_DIR}/adu_types/inc)

add_subdirectory (libaducpal)
add_subdirectory (agent)
add_subdirectory (agent_orchestration)
add_subdirectory (adu-shell)
add_subdirectory (adu_types)
add_subdirectory (adu_workflow)
add_subdirectory (communication_abstraction)
add_subdirectory (communication_managers)
add_subdirectory (diagnostics_component)

if (ADUC_BUILD_DOCUMENTATION)
    add_subdirectory (docs)
endif ()
add_subdirectory (extensions)

add_subdirectory (logging)
add_subdirectory (platform_layers)
add_subdirectory (rootkey_workflow)
add_subdirectory (utils)
