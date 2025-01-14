set(target_name download_file)
add_executable (${target_name})
target_sources (${target_name} PRIVATE main.cpp)

include (agentRules)
target_include_directories (${target_name} PUBLIC ${ADU_EXTENSION_INCLUDES} ${ADUC_EXPORT_INCLUDES})
target_link_dosdk (${target_name} PRIVATE)
target_link_libraries (${target_name} PRIVATE Microsoft::deliveryoptimization atomic)
