set (target_name agent_orchestration)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC "")
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_sources (${target_name} PRIVATE src/agent_orchestration.c)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types
    PRIVATE aduc::c_utils aduc::workflow_utils)
