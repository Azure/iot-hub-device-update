cmake_minimum_required (VERSION 3.5)

project (parson_json_utils)

add_library (${PROJECT_NAME} STATIC src/parson_json_utils.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories (${PROJECT_NAME} PUBLIC inc)

find_package (Parson REQUIRED)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::c_utils Parson::parson
    PRIVATE aduc::logging)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)
target_link_umock_c (${PROJECT_NAME} PRIVATE)
