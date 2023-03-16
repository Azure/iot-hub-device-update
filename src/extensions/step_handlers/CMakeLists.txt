if (NOT ADUC_BUILD_SNAP)
    # Don't need these handlers when build for Ubuntu Core snap.
    add_subdirectory (apt_handler)
    add_subdirectory (simulator_handler)
    add_subdirectory (swupdate_handler)
    add_subdirectory (swupdate_handler_v2)
endif()

add_subdirectory (script_handler)

#
# Note: add more update content handler extensions here.
#
# e.g.
#   add_subdirectory (my_handler)
#
