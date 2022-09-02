set (target_name download_handler_plugin)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

# Turn -fPIC on, in order to use this library in a shared library.
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES}
                                                  ${ADU_EXTENSION_INCLUDES})

target_sources (${target_name} PRIVATE src/download_handler_plugin.cpp)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types
           aduc::contract_utils
           aduc::logging
           aduc::shared_lib)

target_compile_features (${target_name} PRIVATE cxx_lambdas cxx_uniform_initialization)
