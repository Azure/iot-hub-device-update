cmake_minimum_required (VERSION 3.5)

add_subdirectory (curl_downloader)
if (ADUC_BUILD_WITH_DELIVERY_OPTIMIZATION)
    add_subdirectory (deliveryoptimization_downloader)
endif ()
