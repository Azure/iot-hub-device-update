# Check if the ADUC_STEP_HANDLERS list is empty
if (NOT ADUC_STEP_HANDLERS)
    message(FATAL_ERROR "No step handler specified.")
endif()

# Iterate through the step handler list and add the corresponding subdirectories
message("   Step handler to build:")
string(REPLACE "," ";" ADUC_STEP_HANDLERS_LIST ${ADUC_STEP_HANDLERS})
foreach(handler ${ADUC_STEP_HANDLERS_LIST})
    if(handler STREQUAL "")
        message(FATAL_ERROR "Empty step handler specified.")
    endif()

    # Look up the subdirectory name for this step handler
    set(subdirectory "")
    foreach(mapping ${ADUC_STEP_HANDLER_NAME_TO_DIRECOTRY_MAP})
        string(REPLACE "," ";" mapping_parts ${mapping})
        list(GET mapping_parts 0 handler_name)
        list(GET mapping_parts 1 subdirectory_name)
        if(handler STREQUAL ${handler_name})
            set(subdirectory ${subdirectory_name})
            break()
        endif()
    endforeach()

    if(subdirectory STREQUAL "")
        message(FATAL_ERROR "Unknown step handler '${handler}' specified.")
    else()
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${subdirectory})
        message("      handler:${handler} subdirectory:${subdirectory}")
    endif()
endforeach()
