find_package (Doxygen REQUIRED)
if (DOXYGEN_FOUND)
    set (DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/)
    set (DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    set (DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
    set (DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set (DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    message ("Docs INPUT_DIR: ${DOXYGEN_INPUT_DIR}")
    message ("Docs OUTPUT_DIR: ${DOXYGEN_OUTPUT_DIR}")

    configure_file (${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)
    file (MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}) #Doxygen won't create this for us

    add_custom_command (
        OUTPUT ${DOXYGEN_INDEX_FILE}
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
        MAIN_DEPENDENCY ${DOXYFILE_OUT}
        ${DOXYFILE_IN}
        COMMENT "Generating docs")

    add_custom_target (Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})
endif ()
