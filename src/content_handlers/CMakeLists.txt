cmake_minimum_required (VERSION 3.5)

project (content_handlers)

add_library (${PROJECT_NAME} STATIC)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES}
                                                   ${ADU_EXTENSION_INCLUDES})
#
# Note: add ${CMAKE_DL_LIBS} for dynamic library loading support.
#
target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::c_utils
            aduc::exception_utils
            aduc::extension_utils
            aduc::logging
            aduc::string_utils
            aduc::workflow_data_utils
            ${CMAKE_DL_LIBS})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

add_subdirectory (apt_handler)
add_subdirectory (script_handler)
add_subdirectory (simulator_handler)
add_subdirectory (steps_handler)
add_subdirectory (swupdate_handler)
add_subdirectory (swupdate_handler_v2)

#
# Note: add more update content handler extensions here.
#
# e.g.
#   add_subdirectory (my_handler)
#
