
set (target_name sdk)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

target_sources(${target_name} PRIVATE src/aducsdk.c)

target_include_directories (${target_name} PUBLIC inc)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_API_DEFAULT_FIFO_PATH="${ADUC_API_DEFAULT_FIFO_PATH}"
                           ADUC_DATA_FOLDER="${ADUC_DATA_FOLDER}"
                           ADUC_SDK_REQUEST_FIFO_TIMEOUT_SECS=${ADUC_SDK_REQUEST_FIFO_TIMEOUT_SECS})

target_link_libraries (${target_name} PRIVATE aduc::c_utils)

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)
