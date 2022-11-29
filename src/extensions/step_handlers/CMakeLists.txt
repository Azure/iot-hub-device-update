if (NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))
    add_subdirectory (apt_handler)
    add_subdirectory (script_handler)
endif ()

# TODO(JeffMill): WIoT handler needed. Bug on Nox to filter.
add_subdirectory (simulator_handler)

if (NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))
    add_subdirectory (swupdate_handler)
    add_subdirectory (swupdate_handler_v2)
endif ()

#
# Note: add more update content handler extensions here.
#
# e.g.
#   add_subdirectory (my_handler)
#
