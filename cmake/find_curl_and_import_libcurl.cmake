
# Helper function that finds CURL and imports the
# CURL::libcurl target to be used in projects around 
#
macro( find_curl_and_import_libcurl)

    find_package(CURL REQUIRED)

    if (NOT TARGET CURL::libcurl)
        if(CURL_INCLUDE_DIR)
            foreach(_curl_version_header curlver.h curl.h)
                if(EXISTS "${CURL_INCLUDE_DIR}/curl/${_curl_version_header}")
                    file(STRINGS "${CURL_INCLUDE_DIR}/curl/${_curl_version_header}" curl_version_str REGEX "^#define[\t ]+LIBCURL_VERSION[\t ]+\".*\"")

                    string(REGEX REPLACE "^#define[\t ]+LIBCURL_VERSION[\t ]+\"([^\"]*)\".*" "\\1" CURL_VERSION_STRING "${curl_version_str}")
                    unset(curl_version_str)
                    break()
                endif()
            endforeach()
        endif()

        add_library(CURL::libcurl UNKNOWN IMPORTED)
        set_target_properties(CURL::libcurl PROPERTIES
                                INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}")

        if(EXISTS "${CURL_LIBRARY}")
            set_target_properties(CURL::libcurl PROPERTIES
                                    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                                    IMPORTED_LOCATION "${CURL_LIBRARY}")
        endif()
        set_property(TARGET CURL::libcurl APPEND PROPERTY
                        IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(CURL::libcurl PROPERTIES
                                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                                IMPORTED_LOCATION_RELEASE "${CURL_LIBRARY_RELEASE}")
    endif()

endmacro()
