project (content_protection_utils)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (azure_c_shared_utility REQUIRED)
find_package (Parson REQUIRED)

add_library (${PROJECT_NAME} STATIC src/content_protection_utils.c src/decryption_info.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries (${PROJECT_NAME} PUBLIC aduc::c_utils Parson::parson aziotsharedutil)
