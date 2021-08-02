cmake_minimum_required (VERSION 3.5)

project (content_handlers)

add_library (${PROJECT_NAME} STATIC src/content_handler_factory.cpp)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})
target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::logging
            aduc::exception_utils
            aduc::c_utils
            aduc::string_utils)

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
        add_subdirectory (swupdate_handler)
        target_link_libraries (${PROJECT_NAME} PRIVATE aduc::swupdate_handler)
        target_compile_definitions (${PROJECT_NAME} PUBLIC ADUC_SWUPDATE_HANDLER=1)
    elseif (content_handler STREQUAL "microsoft/apt")
        add_subdirectory (apt_handler)
        target_link_libraries (${PROJECT_NAME} PRIVATE aduc::apt_handler)
        target_compile_definitions (${PROJECT_NAME} PUBLIC ADUC_APT_HANDLER=1)
    elseif (content_handler STREQUAL "pantacor/pvcontrol")
        add_subdirectory (pvcontrol_handler)
        target_link_libraries (${PROJECT_NAME} PRIVATE aduc::pvcontrol_handler)
        target_compile_definitions (${PROJECT_NAME} PUBLIC ADUC_PVCONTROL_HANDLER=1)
    else () # NOTE: When adding a new content handler, create a new elseif statement like above.
        message (FATAL_ERROR "Unknown content handler ${content_handler} specified.")
    endif ()
endforeach ()

if (${ADUC_PLATFORM_LAYER} STREQUAL "simulator")
    target_compile_definitions (${PROJECT_NAME} PUBLIC ADUC_SIMULATOR_MODE=1)
endif ()
