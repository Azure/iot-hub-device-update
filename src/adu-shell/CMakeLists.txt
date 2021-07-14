cmake_minimum_required (VERSION 3.5)

project (azure_iot_aduc_shell)

set (target_name adu-shell)

include (agentRules)

compileasc99 ()
disablertti ()

set (agent_c_files ./src/adushell_action.cpp ./src/common_tasks.cpp ./src/main.cpp)

set (agent_apt_c_files ./src/aptget_tasks.cpp)

set (agent_swupdate_c_files ./src/swupdate_tasks.cpp)

set (agent_pvcontrol_c_files ./src/pvcontrol_tasks.cpp)

list (LENGTH ADUC_CONTENT_HANDLERS num_content_handlers)
if (num_content_handlers EQUAL 0)
    message (FATAL_ERROR "No content handler specified.")
endif ()

# Replace comma delimiter with semicolon to form a list
# Converts the list passed in on the command line to a CMake list.

string (
    REPLACE ","
            ";"
            ADUC_CONTENT_HANDLER_LIST
            ${ADUC_CONTENT_HANDLERS})

# Iterate through the list of content handlers and add support for each one.
foreach (content_handler IN LISTS ADUC_CONTENT_HANDLER_LIST)
    if (content_handler STREQUAL "microsoft/swupdate")
        add_executable (${target_name} ${agent_c_files} ${agent_swupdate_c_files})
        set (adushell_def ADUSHELL_SWUPDATE="yes")
    elseif (content_handler STREQUAL "microsoft/apt")
        add_executable (${target_name} ${agent_c_files} ${agent_apt_c_files})
        set (adushell_def ADUSHELL_APT="yes")
    elseif (content_handler STREQUAL "pantacor/pvcontrol")
        add_executable (${target_name} ${agent_c_files} ${agent_pvcontrol_c_files})
        set (adushell_def ADUSHELL_PVCONTROL="yes")
    else () # NOTE: When adding a new content handler, create a new elseif statement like above.
        message (FATAL_ERROR "Unknown content handler ${content_handler} specified.")
    endif ()
endforeach ()

target_include_directories (${target_name} PRIVATE inc ${ADUC_EXPORT_INCLUDES})

set_target_properties (${target_name} PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE)

get_filename_component (
    ADUC_PVINSTALLED_FILE_PATH
    "${ADUC_DATA_FOLDER}/${ADUC_PVINSTALLED_FILE}"
    ABSOLUTE
    "/")
get_filename_component (
    ADUC_PVPROGRESS_FILE_PATH
    "${ADUC_DATA_FOLDER}/${ADUC_PVPROGRESS_FILE}"
    ABSOLUTE
    "/")
target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_VERSION="${ADUC_VERSION}" 
            ADUC_PLATFORM_LAYER="${ADUC_PLATFORM_LAYER}"
            ADUC_CONTENT_HANDLERS="${ADUC_CONTENT_HANDLERS}"
            ADUC_PVINSTALLED_FILE_PATH="${ADUC_PVINSTALLED_FILE_PATH}"
            ADUC_PVPROGRESS_FILE_PATH="${ADUC_PVPROGRESS_FILE_PATH}"
            ${adushell_def})

target_link_libraries (
    ${target_name}
    PRIVATE aduc::logging
            aduc::c_utils
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils)

add_subdirectory (scripts)

# Install adu-shell to /usr/lib/adu folder.
# Only owner and group can run adu-shell.
install (
    TARGETS ${target_name}
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/adu
        PERMISSIONS
            OWNER_EXECUTE
            OWNER_WRITE
            OWNER_READ
            GROUP_READ
            GROUP_EXECUTE)
