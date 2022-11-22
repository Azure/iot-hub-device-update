cmake_minimum_required (VERSION 3.5)

project (content_downloaders)

if (NOT ADUC_UBUNTU_CORE_SNAP_ONLY)
    # For Ubuntu Core snap MVP, delivery optimization is not supported.
    add_subdirectory (deliveryoptimization_downloader)
endif()

add_subdirectory (curl_downloader)
