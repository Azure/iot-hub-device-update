/**
 * @file mock_steps_handler_deps.cpp
 * @brief Mock implementations of all external dependencies used by steps_handler.cpp
 *        and handler_create.cpp so they can be tested in isolation.
 */
#include "mock_steps_handler_deps.h"

#include <aduc/content_handler.hpp>
#include <aduc/contract_utils.h>
#include <aduc/extension_manager.hpp>
#include <aduc/extension_manager_download_options.h>
#include <aduc/logging.h>
#include <aduc/parser_utils.h>
#include <aduc/string_c_utils.h>
#include <aduc/system_utils.h>
#include <aduc/types/update_content.h>
#include <aduc/types/workflow.h>
#include <aduc/workflow_utils.h>

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

/* =====================================================================
 * Mock state variables
 * ===================================================================== */

bool mock_is_cancel_requested = false;
int mock_workflow_level = 0;
int mock_workflow_step = 0;
const char* mock_workflow_id = "test-wf-id";
bool mock_request_cancel_return = true;
const char* mock_workfolder = "/tmp/test-workfolder";

int mock_instructions_steps_count = 0;
size_t mock_children_count = 0;
bool mock_is_inline_step = true;
const char* mock_selected_components = nullptr;
bool mock_set_selected_components_return = true;
ADUC_Result mock_create_from_inline_step_result = { 1, 0 }; /* success */
bool mock_get_step_detached_manifest_return = false;
ADUC_Result mock_init_from_file_result = { 1, 0 };
bool mock_insert_child_return = true;
const char* mock_step_handler_update_type = "microsoft/test:1";
const char* mock_compat_string = nullptr;

int mock_mksandbox_return = 0;

ADUC_Result mock_ext_download_result = { 1, 0 };
ADUC_Result mock_load_handler_result = { 1, 0 };
bool mock_is_components_enumerator_registered = false;
ADUC_Result mock_select_components_result = { 1, 0 };
const char* mock_select_components_output = nullptr;

ADUC_Result mock_handler_download_result = { ADUC_Result_Download_Success, 0 };
ADUC_Result mock_handler_install_result = { ADUC_Result_Install_Success, 0 };
ADUC_Result mock_handler_apply_result = { ADUC_Result_Apply_Success, 0 };
ADUC_Result mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
ADUC_Result mock_handler_restore_result = { ADUC_Result_Restore_Success, 0 };
ADUC_Result mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
bool mock_handler_is_installed_throws = false;

bool mock_immediate_reboot_requested = false;
bool mock_immediate_agent_restart_requested = false;
bool mock_reboot_requested = false;
bool mock_agent_restart_requested = false;

int mock_contract_major = ADUC_V1_CONTRACT_MAJOR_VER;
int mock_contract_minor = ADUC_V1_CONTRACT_MINOR_VER;

/* =====================================================================
 * Child handle tracking
 * ===================================================================== */
#define MAX_MOCK_CHILDREN 16
static void* s_child_handles[MAX_MOCK_CHILDREN];
static int s_child_handle_count = 0;

/* Fake addressable memory blocks used as child workflow "handles" */
static char s_fake_child_mem[MAX_MOCK_CHILDREN];
static int s_fake_child_alloc = 0;

/* =====================================================================
 * Reset function
 * ===================================================================== */

void mock_steps_handler_reset(void)
{
    mock_is_cancel_requested = false;
    mock_workflow_level = 0;
    mock_workflow_step = 0;
    mock_workflow_id = "test-wf-id";
    mock_request_cancel_return = true;
    mock_workfolder = "/tmp/test-workfolder";

    mock_instructions_steps_count = 0;
    mock_children_count = 0;
    mock_is_inline_step = true;
    mock_selected_components = nullptr;
    mock_set_selected_components_return = true;
    mock_create_from_inline_step_result = { 1, 0 };
    mock_get_step_detached_manifest_return = false;
    mock_init_from_file_result = { 1, 0 };
    mock_insert_child_return = true;
    mock_step_handler_update_type = "microsoft/test:1";
    mock_compat_string = nullptr;

    mock_mksandbox_return = 0;

    mock_ext_download_result = { 1, 0 };
    mock_load_handler_result = { 1, 0 };
    mock_is_components_enumerator_registered = false;
    mock_select_components_result = { 1, 0 };
    mock_select_components_output = nullptr;

    mock_handler_download_result = { ADUC_Result_Download_Success, 0 };
    mock_handler_install_result = { ADUC_Result_Install_Success, 0 };
    mock_handler_apply_result = { ADUC_Result_Apply_Success, 0 };
    mock_handler_backup_result = { ADUC_Result_Backup_Success, 0 };
    mock_handler_restore_result = { ADUC_Result_Restore_Success, 0 };
    mock_handler_is_installed_result = { ADUC_Result_IsInstalled_NotInstalled, 0 };
    mock_handler_is_installed_throws = false;

    mock_immediate_reboot_requested = false;
    mock_immediate_agent_restart_requested = false;
    mock_reboot_requested = false;
    mock_agent_restart_requested = false;

    mock_contract_major = ADUC_V1_CONTRACT_MAJOR_VER;
    mock_contract_minor = ADUC_V1_CONTRACT_MINOR_VER;

    s_child_handle_count = 0;
    s_fake_child_alloc = 0;
    memset(s_child_handles, 0, sizeof(s_child_handles));
    memset(s_fake_child_mem, 0, sizeof(s_fake_child_mem));
}

/* =====================================================================
 * Default_ExtensionManager_Download_Options (extern in steps_handler.cpp)
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

char* workflow_get_workfolder(ADUC_WorkflowHandle)
{
    return strdup(mock_workfolder);
}

void workflow_free_string(char* str)
{
    free(str);
}

size_t workflow_get_instructions_steps_count(ADUC_WorkflowHandle)
{
    return static_cast<size_t>(mock_instructions_steps_count);
}

size_t workflow_get_children_count(ADUC_WorkflowHandle)
{
    return static_cast<size_t>(s_child_handle_count);
}

ADUC_WorkflowHandle workflow_remove_child(ADUC_WorkflowHandle, int index)
{
    if (index < s_child_handle_count)
    {
        void* child = s_child_handles[index];
        for (int i = index; i < s_child_handle_count - 1; i++)
        {
            s_child_handles[i] = s_child_handles[i + 1];
        }
        s_child_handle_count--;
        return child;
    }
    return nullptr;
}

void workflow_free(ADUC_WorkflowHandle)
{
    /* no-op */
}

bool workflow_is_inline_step(ADUC_WorkflowHandle, size_t)
{
    return mock_is_inline_step;
}

char* workflow_get_selected_components(ADUC_WorkflowHandle)
{
    if (mock_selected_components == nullptr)
    {
        return nullptr;
    }
    return strdup(mock_selected_components);
}

ADUC_Result workflow_create_from_inline_step(ADUC_WorkflowHandle, size_t, ADUC_WorkflowHandle* child)
{
    if (IsAducResultCodeSuccess(mock_create_from_inline_step_result.ResultCode))
    {
        *child = &s_fake_child_mem[s_fake_child_alloc++];
    }
    else
    {
        *child = nullptr;
    }
    return mock_create_from_inline_step_result;
}

void workflow_set_step_index(ADUC_WorkflowHandle, size_t)
{
    /* no-op */
}

bool workflow_set_selected_components(ADUC_WorkflowHandle, const char*)
{
    return mock_set_selected_components_return;
}

bool workflow_get_step_detached_manifest_file(ADUC_WorkflowHandle, size_t, ADUC_FileEntity* entity)
{
    if (!mock_get_step_detached_manifest_return)
    {
        return false;
    }
    memset(entity, 0, sizeof(*entity));
    entity->FileId = strdup("test-file-id");
    entity->TargetFilename = strdup("detached-manifest.json");
    return true;
}

ADUC_Result workflow_init_from_file(const char*, bool, ADUC_WorkflowHandle* handle)
{
    if (IsAducResultCodeSuccess(mock_init_from_file_result.ResultCode))
    {
        *handle = &s_fake_child_mem[s_fake_child_alloc++];
    }
    else
    {
        *handle = nullptr;
    }
    return mock_init_from_file_result;
}

char* workflow_get_update_manifest_compatibility(ADUC_WorkflowHandle, size_t)
{
    if (mock_compat_string == nullptr)
    {
        return nullptr;
    }
    return strdup(mock_compat_string);
}

bool workflow_set_id(ADUC_WorkflowHandle, const char*)
{
    return true;
}

bool workflow_insert_child(ADUC_WorkflowHandle, int, ADUC_WorkflowHandle child)
{
    if (!mock_insert_child_return)
    {
        return false;
    }
    if (s_child_handle_count < MAX_MOCK_CHILDREN)
    {
        s_child_handles[s_child_handle_count++] = child;
        return true;
    }
    return false;
}

ADUC_WorkflowHandle workflow_get_child(ADUC_WorkflowHandle, size_t index)
{
    if (static_cast<int>(index) < s_child_handle_count)
    {
        return s_child_handles[index];
    }
    return nullptr;
}

const char* workflow_peek_update_manifest_step_handler(ADUC_WorkflowHandle, size_t)
{
    return mock_step_handler_update_type;
}

void workflow_set_result(ADUC_WorkflowHandle, ADUC_Result)
{
    /* no-op */
}

const char* workflow_peek_result_details(ADUC_WorkflowHandle)
{
    return "mock-result-details";
}

void workflow_set_result_details(ADUC_WorkflowHandle, const char*, ...)
{
    /* no-op */
}

bool workflow_set_state(ADUC_WorkflowHandle, ADUCITF_State)
{
    return true;
}

ADUC_Result workflow_get_result(ADUC_WorkflowHandle)
{
    return { 0, 0 };
}

bool workflow_request_cancel(ADUC_WorkflowHandle)
{
    return mock_request_cancel_return;
}

bool workflow_request_immediate_reboot(ADUC_WorkflowHandle)
{
    return true;
}

bool workflow_request_immediate_agent_restart(ADUC_WorkflowHandle)
{
    return true;
}

bool workflow_is_immediate_reboot_requested(ADUC_WorkflowHandle)
{
    return mock_immediate_reboot_requested;
}

bool workflow_is_immediate_agent_restart_requested(ADUC_WorkflowHandle)
{
    return mock_immediate_agent_restart_requested;
}

bool workflow_request_reboot(ADUC_WorkflowHandle)
{
    return true;
}

bool workflow_request_agent_restart(ADUC_WorkflowHandle)
{
    return true;
}

bool workflow_is_reboot_requested(ADUC_WorkflowHandle)
{
    return mock_reboot_requested;
}

bool workflow_is_agent_restart_requested(ADUC_WorkflowHandle)
{
    return mock_agent_restart_requested;
}

char* workflow_get_serialized_update_manifest(ADUC_WorkflowHandle, bool)
{
    return strdup("{}");
}

/* --- ADUC_FileEntity_Uninit --- */
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

/* --- ADUC_SystemUtils_MkSandboxDirRecursive --- */
int ADUC_SystemUtils_MkSandboxDirRecursive(const char*)
{
    return mock_mksandbox_return;
}

/* --- ADUC_ContractUtils_IsV1Contract --- */
bool ADUC_ContractUtils_IsV1Contract(ADUC_ExtensionContractInfo*)
{
    return (mock_contract_major == ADUC_V1_CONTRACT_MAJOR_VER
            && mock_contract_minor == ADUC_V1_CONTRACT_MINOR_VER);
}

/* --- IsNullOrEmpty --- */
bool IsNullOrEmpty(const char* str)
{
    return (str == nullptr || *str == '\0');
}

/* --- Logging stubs --- */
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

} /* extern "C" */

/* =====================================================================
 * MockContentHandler — implements ContentHandler interface
 * ===================================================================== */

class MockContentHandler : public ContentHandler
{
public:
    MockContentHandler() = default;

    ADUC_Result Download(const tagADUC_WorkflowData*) override
    {
        return mock_handler_download_result;
    }

    ADUC_Result Backup(const tagADUC_WorkflowData*) override
    {
        return mock_handler_backup_result;
    }

    ADUC_Result Install(const tagADUC_WorkflowData*) override
    {
        return mock_handler_install_result;
    }

    ADUC_Result Apply(const tagADUC_WorkflowData*) override
    {
        return mock_handler_apply_result;
    }

    ADUC_Result Restore(const tagADUC_WorkflowData*) override
    {
        return mock_handler_restore_result;
    }

    ADUC_Result Cancel(const tagADUC_WorkflowData*) override
    {
        return { ADUC_Result_Cancel_Success, 0 };
    }

    ADUC_Result IsInstalled(const tagADUC_WorkflowData*) override
    {
        if (mock_handler_is_installed_throws)
        {
            throw std::runtime_error("mock IsInstalled exception");
        }
        return mock_handler_is_installed_result;
    }
};

static MockContentHandler s_mock_content_handler;

/* =====================================================================
 * ExtensionManager static member variable definitions
 * ===================================================================== */

std::unordered_map<std::string, void*> ExtensionManager::_libs;
std::unordered_map<std::string, ContentHandler*> ExtensionManager::_contentHandlers;
void* ExtensionManager::_contentDownloader = nullptr;
ADUC_ExtensionContractInfo ExtensionManager::_contentDownloaderContractVersion = {};
void* ExtensionManager::_componentEnumerator = nullptr;
ADUC_ExtensionContractInfo ExtensionManager::_componentEnumeratorContractVersion = {};

/* =====================================================================
 * ExtensionManager mock static method implementations
 * ===================================================================== */

ADUC_Result ExtensionManager::Download(
    const ADUC_FileEntity*,
    ADUC_WorkflowHandle,
    ExtensionManager_Download_Options*,
    ADUC_DownloadProgressCallback,
    ADUC_DownloadProcResolver)
{
    return mock_ext_download_result;
}

DownloadProc ExtensionManager::DefaultDownloadProcResolver(void*)
{
    return nullptr;
}

ADUC_Result ExtensionManager::LoadUpdateContentHandlerExtension(const std::string&, ContentHandler** handler)
{
    if (handler != nullptr && IsAducResultCodeSuccess(mock_load_handler_result.ResultCode))
    {
        /* Set contract info on the mock handler so GetContractInfo() returns our mock values */
        ADUC_ExtensionContractInfo ci;
        ci.majorVer = mock_contract_major;
        ci.minorVer = mock_contract_minor;
        s_mock_content_handler.SetContractInfo(ci);
        *handler = &s_mock_content_handler;
    }
    return mock_load_handler_result;
}

bool ExtensionManager::IsComponentsEnumeratorRegistered()
{
    return mock_is_components_enumerator_registered;
}

ADUC_Result ExtensionManager::SelectComponents(const std::string&, std::string& output)
{
    if (mock_select_components_output != nullptr)
    {
        output = mock_select_components_output;
    }
    return mock_select_components_result;
}

void ExtensionManager::Uninit()
{
    /* no-op */
}

void ExtensionManager::UnloadAllUpdateContentHandlers()
{
    /* no-op */
}

void ExtensionManager::UnloadAllExtensions()
{
    /* no-op */
}

/* Remaining required static methods — stubs */
ADUC_Result ExtensionManager::LoadContentDownloaderLibrary(void**)
{
    return { 0, 0 };
}

ADUC_Result ExtensionManager::SetContentDownloaderLibrary(void*)
{
    return { 0, 0 };
}

ADUC_Result ExtensionManager::GetContentDownloaderContractVersion(ADUC_ExtensionContractInfo*)
{
    return { 0, 0 };
}

void ExtensionManager::SetContentDownloaderContractVersion(const ADUC_ExtensionContractInfo&)
{
}

ADUC_Result ExtensionManager::LoadComponentEnumeratorLibrary(void**)
{
    return { 0, 0 };
}

ADUC_Result ExtensionManager::GetComponentEnumeratorContractVersion(ADUC_ExtensionContractInfo*)
{
    return { 0, 0 };
}

ADUC_Result ExtensionManager::SetUpdateContentHandlerExtension(const std::string&, ContentHandler*)
{
    return { 0, 0 };
}

ADUC_Result ExtensionManager::GetAllComponents(std::string&)
{
    return { 0, 0 };
}

ADUC_Result ExtensionManager::InitializeContentDownloader(const char*)
{
    return { 0, 0 };
}

ADUC_Result ExtensionManager::LoadExtensionLibrary(
    const char*, const char*, const char*, const char*, const char*, int, int, void**)
{
    return { 0, 0 };
}

void ExtensionManager::_FreeComponentsDataString(char*)
{
}
