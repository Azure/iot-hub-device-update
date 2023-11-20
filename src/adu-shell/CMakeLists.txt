cmake_minimum_required (VERSION 3.5)

set (target_name adu-shell)

include (agentRules)

compileasc99 ()
disablertti ()

set (agent_c_files ./src/adushell_action.cpp ./src/common_tasks.cpp ./src/main.cpp)

set (agent_apt_c_files ./src/aptget_tasks.cpp)

set (agent_script_c_files ./src/script_tasks.cpp)

list (LENGTH ADUC_STEP_HANDLERS num_step_handlers)
if (num_step_handlers EQUAL 0)
    message (FATAL_ERROR "No step handlers specified.")
endif ()

# Replace comma delimiter with semicolon to form a list
# Converts the list passed in on the command line to a CMake list.

string (
    REPLACE ","
            ";"
            ADUC_STEP_HANDLER_LIST
            ${ADUC_STEP_HANDLERS})

# Support all microsoft/* update types by default.
set (source_files ${agent_c_files} ${agent_apt_c_files} ${agent_script_c_files})

set (adushell_def ADUSHELL_SWUPDATE="yes" ADUSHELL_APT="yes" ADUSHELL_SCRIPT="yes")

add_executable (${target_name} ${source_files})

target_include_directories (${target_name} PRIVATE inc ${ADUC_EXPORT_INCLUDES})

set_target_properties (${target_name} PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE)

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_VERSION="${ADUC_VERSION}"
            ADUC_PLATFORM_LAYER="${ADUC_PLATFORM_LAYER}"
            ADUC_CONF_FOLDER="${ADUC_CONF_FOLDER}"
            ADUC_STEP_HANDLERS="${ADUC_STEP_HANDLERS}"
            ADUSHELL_EFFECTIVE_GROUP_NAME="${ADUSHELL_EFFECTIVE_GROUP_NAME}"
            ${adushell_def})

target_link_aziotsharedutil (${target_name} PRIVATE)

if (WIN32)
    find_package (unofficial-getopt-win32 REQUIRED)
    target_link_libraries (${target_name} PRIVATE unofficial::getopt-win32::getopt)
endif ()

target_link_libraries (
    ${target_name}
    PRIVATE aduc::logging
            aduc::c_utils
            aduc::config_utils
            aduc::process_utils
            aduc::string_utils)

add_subdirectory (scripts)

# Install adu-shell to /usr/bin folder.
# Only owner and group can run adu-shell.
install (
    TARGETS ${target_name}
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PERMISSIONS
            OWNER_EXECUTE
            OWNER_WRITE
            OWNER_READ
            GROUP_READ
            GROUP_EXECUTE)
