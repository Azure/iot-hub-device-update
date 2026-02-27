/**
 * @file mock_simulator_handler_deps.cpp
 * @brief Mock implementations of all external dependencies used by simulator_handler.cpp.
 */
#include "mock_simulator_handler_deps.h"

#include <aduc/content_handler.hpp>
#include <aduc/logging.h>
#include <aduc/parser_utils.h>
#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <aduc/workflow_utils.h>

#include <cstring>
#include <string>

/* =====================================================================
 * Mock state variables
 * ===================================================================== */

size_t mock_update_files_count = 1;
bool mock_get_update_file_return = true;
const char* mock_target_filename = "test-file.json";
const char* mock_installed_criteria = "test-criteria";

void mock_simulator_handler_reset(void)
{
    mock_update_files_count = 1;
    mock_get_update_file_return = true;
    mock_target_filename = "test-file.json";
    mock_installed_criteria = "test-criteria";
}

/* =====================================================================
 * workflow_* mock implementations
 * ===================================================================== */

extern "C"
{

size_t workflow_get_update_files_count(ADUC_WorkflowHandle)
{
    return mock_update_files_count;
}

bool workflow_get_update_file(ADUC_WorkflowHandle, size_t, ADUC_FileEntity* entity)
{
    if (!mock_get_update_file_return)
    {
        return false;
    }
    memset(entity, 0, sizeof(*entity));
    entity->TargetFilename = strdup(mock_target_filename);
    return true;
}

void workflow_free_file_entity(ADUC_FileEntity* entity)
{
    if (entity != nullptr)
    {
        free(entity->TargetFilename);
        entity->TargetFilename = nullptr;
    }
}

char* workflow_get_installed_criteria(ADUC_WorkflowHandle)
{
    if (mock_installed_criteria == nullptr)
    {
        return nullptr;
    }
    return strdup(mock_installed_criteria);
}

void workflow_set_result_details(ADUC_WorkflowHandle, const char*, ...)
{
    /* no-op */
}

const char* workflow_peek_result_details(ADUC_WorkflowHandle)
{
    return "";
}

void workflow_free_string(char* str)
{
    free(str);
}

void ADUC_FileEntity_Uninit(ADUC_FileEntity* entity)
{
    if (entity != nullptr)
    {
        free(entity->TargetFilename);
        entity->TargetFilename = nullptr;
        free(entity->FileId);
        entity->FileId = nullptr;
    }
}

} /* extern "C" */

/* =====================================================================
 * Logging stubs
 * ===================================================================== */

ADUC_LOG_SEVERITY ADUC_Logging_GetLevel(void)
{
    return ADUC_LOG_INFO;
}

void ADUC_Logging_Init(ADUC_LOG_SEVERITY, const char*)
{
    /* no-op */
}

void ADUC_Logging_Uninit(void)
{
    /* no-op */
}

void zlog_log(enum ZLOG_SEVERITY, const char*, unsigned int, const char*, ...)
{
    /* no-op */
}
