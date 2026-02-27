/**
 * @file mock_swupdate_handler_deps.cpp
 * @brief Mock implementations of all external dependencies used by swupdate_handler_v2.cpp
 *        and handler_create.cpp.
 */
#include "mock_swupdate_handler_deps.h"

#include <aduc/config_utils.h>
#include <aduc/content_handler.hpp>
#include <aduc/extension_manager.hpp>
#include <aduc/extension_manager_download_options.h>
#include <aduc/logging.h>
#include <aduc/parser_utils.h>
#include <aduc/process_utils.hpp>
#include <aduc/result.h>
#include <aduc/string_c_utils.h>
#include <aduc/system_utils.h>
#include <aduc/types/update_content.h>
#include <aduc/workflow_data_utils.h>
#include <aduc/workflow_utils.h>

#include <cstdarg>
#include <cstring>
#include <string>
#include <vector>

/* =====================================================================
 * Mock state variables
 * ===================================================================== */

bool mock_is_cancel_requested = false;
size_t mock_update_files_count = 2;
bool mock_get_update_file_return = true;
bool mock_get_update_file_by_name_return = true;
const char* mock_workfolder = "/tmp/test-workfolder";
const char* mock_installed_criteria = "test-criteria-1.0";
const char* mock_workflow_id = "test-wf-id";
int mock_workflow_level = 0;
int mock_workflow_step = 0;
bool mock_request_cancel_return = true;
const char* mock_target_filename = "test-script.sh";
const char* mock_selected_components = nullptr;
const char* mock_handler_property_scriptfilename = "test-script.sh";
const char* mock_handler_property_swufilename = "test.swu";
const char* mock_handler_property_apiversion = nullptr;
const char* mock_handler_property_arguments = nullptr;

bool mock_config_available = true;
const char* mock_adu_shell_path = "/usr/bin/adu-shell";
const char* mock_config_folder = "/etc/adu";

ADUC_Result mock_extension_download_result = { 1, 0 };

int mock_launch_child_exit_code = 0;
int mock_launch_child_call_count = 0;

int mock_mkdir_result = 0;

const char* mock_workdata_workfolder = "/tmp/test-workfolder";

void mock_swupdate_handler_reset(void)
{
    mock_is_cancel_requested = false;
    mock_update_files_count = 2;
    mock_get_update_file_return = true;
    mock_get_update_file_by_name_return = true;
    mock_workfolder = "/tmp/test-workfolder";
    mock_installed_criteria = "test-criteria-1.0";
    mock_workflow_id = "test-wf-id";
    mock_workflow_level = 0;
    mock_workflow_step = 0;
    mock_request_cancel_return = true;
    mock_target_filename = "test-script.sh";
    mock_selected_components = nullptr;
    mock_handler_property_scriptfilename = "test-script.sh";
    mock_handler_property_swufilename = "test.swu";
    mock_handler_property_apiversion = nullptr;
    mock_handler_property_arguments = nullptr;
    mock_config_available = true;
    mock_adu_shell_path = "/usr/bin/adu-shell";
    mock_config_folder = "/etc/adu";
    mock_extension_download_result = { 1, 0 };
    mock_launch_child_exit_code = 0;
    mock_launch_child_call_count = 0;
    mock_mkdir_result = 0;
    mock_workdata_workfolder = "/tmp/test-workfolder";
}

/* =====================================================================
 * Default_ExtensionManager_Download_Options
 * ===================================================================== */

ExtensionManager_Download_Options Default_ExtensionManager_Download_Options = { 480 };

/* =====================================================================
 * workflow_* mock implementations
 * ===================================================================== */

extern "C"
{

bool workflow_is_cancel_requested(ADUC_WorkflowHandle)
{
    return mock_is_cancel_requested;
}

size_t workflow_get_update_files_count(ADUC_WorkflowHandle)
{
    return mock_update_files_count;
}

char* workflow_get_workfolder(ADUC_WorkflowHandle)
{
    return strdup(mock_workfolder);
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

bool workflow_get_update_file_by_name(ADUC_WorkflowHandle, const char*, ADUC_FileEntity* entity)
{
    if (!mock_get_update_file_by_name_return)
    {
        return false;
    }
    memset(entity, 0, sizeof(*entity));
    entity->TargetFilename = strdup(mock_target_filename);
    return true;
}

char* workflow_get_installed_criteria(ADUC_WorkflowHandle)
{
    if (mock_installed_criteria == nullptr)
    {
        return nullptr;
    }
    return strdup(mock_installed_criteria);
}

char* workflow_get_selected_components(ADUC_WorkflowHandle)
{
    if (mock_selected_components == nullptr)
    {
        return nullptr;
    }
    return strdup(mock_selected_components);
}

const char* workflow_peek_update_manifest_handler_properties_string(ADUC_WorkflowHandle, const char* propertyName)
{
    if (strcmp(propertyName, "scriptFileName") == 0)
    {
        return mock_handler_property_scriptfilename;
    }
    if (strcmp(propertyName, "apiVersion") == 0)
    {
        return mock_handler_property_apiversion;
    }
    if (strcmp(propertyName, "swuFileName") == 0)
    {
        return mock_handler_property_swufilename;
    }
    if (strcmp(propertyName, "arguments") == 0)
    {
        return mock_handler_property_arguments;
    }
    return nullptr;
}

void workflow_set_result_details(ADUC_WorkflowHandle, const char*, ...)
{
    /* no-op */
}

void workflow_free_string(char* str)
{
    free(str);
}

bool workflow_get_operation_cancel_requested(ADUC_WorkflowHandle)
{
    return false;
}

bool workflow_request_cancel(ADUC_WorkflowHandle)
{
    return mock_request_cancel_return;
}

bool workflow_request_immediate_reboot(ADUC_WorkflowHandle)
{
    return true;
}

bool workflow_request_reboot(ADUC_WorkflowHandle)
{
    return true;
}

bool workflow_request_immediate_agent_restart(ADUC_WorkflowHandle)
{
    return true;
}

bool workflow_request_agent_restart(ADUC_WorkflowHandle)
{
    return true;
}

int workflow_get_level(ADUC_WorkflowHandle)
{
    return mock_workflow_level;
}

int workflow_get_step_index(ADUC_WorkflowHandle)
{
    return mock_workflow_step;
}

const char* workflow_peek_id(ADUC_WorkflowHandle)
{
    return mock_workflow_id;
}

ADUC_Result workflow_get_result(ADUC_WorkflowHandle)
{
    return { 0, 0 };
}

void workflow_set_result(ADUC_WorkflowHandle, ADUC_Result)
{
    /* no-op */
}

bool workflow_set_state(ADUC_WorkflowHandle, ADUCITF_State)
{
    return true;
}

const char* workflow_peek_result_details(ADUC_WorkflowHandle)
{
    return "";
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

int ADUC_SystemUtils_MkSandboxDirRecursive(const char*)
{
    return mock_mkdir_result;
}

char* ADUC_WorkflowData_GetWorkFolder(const ADUC_WorkflowData*)
{
    if (mock_workdata_workfolder == nullptr)
    {
        return nullptr;
    }
    return strdup(mock_workdata_workfolder);
}

} /* extern "C" */

/* =====================================================================
 * ADUC_ConfigInfo mock
 * ===================================================================== */

static ADUC_ConfigInfo s_mock_config = {};

const ADUC_ConfigInfo* ADUC_ConfigInfo_GetInstance()
{
    if (!mock_config_available)
    {
        return nullptr;
    }
    s_mock_config.aduShellFilePath = strdup(mock_adu_shell_path);
    s_mock_config.configFolder = strdup(mock_config_folder);
    return &s_mock_config;
}

int ADUC_ConfigInfo_ReleaseInstance(const ADUC_ConfigInfo* config)
{
    if (config == &s_mock_config)
    {
        free(s_mock_config.aduShellFilePath);
        s_mock_config.aduShellFilePath = nullptr;
        free(s_mock_config.configFolder);
        s_mock_config.configFolder = nullptr;
    }
    return 0;
}

/* =====================================================================
 * ExtensionManager mock
 * ===================================================================== */

std::unordered_map<std::string, void*> ExtensionManager::_libs;
std::unordered_map<std::string, ContentHandler*> ExtensionManager::_contentHandlers;
void* ExtensionManager::_contentDownloader = nullptr;
ADUC_ExtensionContractInfo ExtensionManager::_contentDownloaderContractVersion = {};
void* ExtensionManager::_componentEnumerator = nullptr;
ADUC_ExtensionContractInfo ExtensionManager::_componentEnumeratorContractVersion = {};

DownloadProc ExtensionManager::DefaultDownloadProcResolver(void*)
{
    return nullptr;
}

ADUC_Result ExtensionManager::Download(
    const ADUC_FileEntity*,
    ADUC_WorkflowHandle,
    ExtensionManager_Download_Options*,
    ADUC_DownloadProgressCallback,
    ADUC_DownloadProcResolver)
{
    return mock_extension_download_result;
}

/* =====================================================================
 * ADUC_LaunchChildProcess mock
 * ===================================================================== */

int ADUC_LaunchChildProcess(const std::string&, std::vector<std::string>, std::string&)
{
    mock_launch_child_call_count++;
    return mock_launch_child_exit_code;
}

/* =====================================================================
 * IsNullOrEmpty
 * ===================================================================== */

bool IsNullOrEmpty(const char* str)
{
    return (str == nullptr || *str == '\0');
}

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
