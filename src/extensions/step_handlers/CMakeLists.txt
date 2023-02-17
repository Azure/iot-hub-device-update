add_subdirectory (script_handler)
add_subdirectory (simulator_handler)

if (NOT WIN32)
    add_subdirectory (apt_handler)
    add_subdirectory (swupdate_handler)
    add_subdirectory (swupdate_handler_v2)
else ()
    add_subdirectory (wim_handler)
endif ()

#
# Note: add more update content handler extensions here.
#
# e.g.
#   add_subdirectory (my_handler)
#
