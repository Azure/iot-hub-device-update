find_package (Doxygen REQUIRED)
if (DOXYGEN_FOUND)
    set (DOXYGEN_PROJECT_NAME "Device Update for IotHub")
    set (DOXYGEN_PROJECT_NUMBER "1.0.2")
    set (
        DOXYGEN_PROJECT_BRIEF
        "Device Update for IoTHub delivers client update from the Device Update for IotHub service."
    )
    set (DOXYGEN_EXCLUDE_PATTERNS "*/src/deliveryoptimization/* */tests/* *.sh *.ps1 *.cpp *.c")

    set (DOXYGEN_GENERATE_HTML YES)
    set (DOXYGEN_GENERATE_MAN YES)
    doxygen_add_docs (doxygen_docs ${PROJECT_SOURCE_DIR} ALL COMMENT "Generating doxygen")
endif ()
