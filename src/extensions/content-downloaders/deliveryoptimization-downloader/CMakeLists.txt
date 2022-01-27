project (deliveryoptimization-content-downloader)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} SHARED deliveryoptimization-content-downloader.cpp)

find_package (deliveryoptimization_sdk CONFIG REQUIRED)

add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC ${ADUC_EXTENSION_INCLUDES}
                                                   ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aziotsharedutil
            aduc::c_utils
            aduc::hash_utils
            aduc::logging
            aduc::process_utils
            aduc::string_utils
            Microsoft::deliveryoptimization)

install (TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
