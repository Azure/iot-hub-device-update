cmake_minimum_required (VERSION 3.5)

project (content_downloaders)

# TODO(JeffMill): create a WIoT DO downloader. Bug against Nox on not being able to filter.
add_subdirectory (curl_downloader)
if (NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))
    add_subdirectory (deliveryoptimization_downloader)
endif ()
