# TODO(JeffMill): Unable to filter handlers at build time:
# Task 42405545: [P1] No way to selectively build content downloaders should I remove ADUC_CONTENT_HANDLERS for now?
# Task 42398328: [P1] add the --content-handlers option back.

if (NOT WIN32)
    add_subdirectory (apt_handler)
    add_subdirectory (script_handler)
    add_subdirectory (simulator_handler)
    add_subdirectory (swupdate_handler)
    add_subdirectory (swupdate_handler_v2)
else ()
    add_subdirectory (simulator_handler)
    add_subdirectory (wiot_handler)
endif ()

#
# Note: add more update content handler extensions here.
#
# e.g.
#   add_subdirectory (my_handler)
#
