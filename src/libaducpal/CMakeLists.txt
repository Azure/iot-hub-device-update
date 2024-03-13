cmake_minimum_required (VERSION 3.5)

set (target_name libaducpal)
# CMake Note:  We do not recommend using GLOB to collect a list of source files from your source tree.
# If no CMakeLists.txt file changes when a source is added or removed then the generated build system
# cannot know when to ask CMake to regenerate.

if (WIN32)

    # Build as a static library
    add_library (
        ${target_name} STATIC
        "src/dirent.c"
        "src/dlfcn.c"
        "src/ftw.c"
        "src/grp.c"
        "src/pwd.c"
        "src/stdio.c"
        "src/stdlib.c"
        "src/strings.c"
        "src/sys_stat.c"
        "src/sys_time.c"
        "src/time.c"
        "src/unistd.c")

    # Ensure that this project is built with secure and standard CRT APIs.
    remove_definitions (-D_CRT_SECURE_NO_WARNINGS)
    remove_definitions (-D_CRT_NONSTDC_NO_WARNINGS)

    # On Windows, any project that links to this will have ADUCPAL_USE_PAL defined
    # so that the headers bring in the correct includes and function definitions.
    target_compile_definitions (${target_name} PUBLIC ADUCPAL_USE_PAL)

    # Ensure that all projects (including this) have access to this project's inc folder.
    target_include_directories (${target_name} PUBLIC "inc")

else ()

    # Build as header-only library
    add_library (${target_name} INTERFACE)

    target_include_directories (${target_name} INTERFACE "inc")
endif ()
