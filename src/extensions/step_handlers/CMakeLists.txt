add_subdirectory (script_handler)
add_subdirectory (simulator_handler)
add_subdirectory (swupdate_handler)
add_subdirectory (swupdate_handler_v2)

if (NOT WIN32)
    add_subdirectory (apt_handler)
else ()
    add_subdirectory (wiot_handler)
endif ()

#
# Note: add more update content handler extensions here.
#
# e.g.
#   add_subdirectory (my_handler)
#
