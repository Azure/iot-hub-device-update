project (adushell_utils)

add_library (${PROJECT_NAME} STATIC src/adushell_const.cpp)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc)
