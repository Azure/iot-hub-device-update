cmake_minimum_required(VERSION 3.14)
project(delta_processor_unit_tests)

find_package(Catch2 REQUIRED)

add_executable(${PROJECT_NAME}
    delta_processor_ut.cpp
)

# Link the delta_processor as an OBJECT library for testing (access internal symbols)
# We compile the source directly so ADUC_GetExtensionDescriptor is available.
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../delta_processor.c
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE aduc::extension_sdk
    PRIVATE Catch2::Catch2WithMain
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})
