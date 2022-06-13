#include "aduc/download_handler_plugin.h"
#include "aduc/download_handler_plugin.hpp"

#include <aduc/logging.h>
#include <memory>

using InitializeFn = void (*)(ADUC_LOG_SEVERITY logLevel);

using CleanupFn = void (*)();

using ProcessUpdateFn = ADUC_Result (*)(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* fileEntity, const char* targetFilePath);

using OnUpdateWorkflowCompletedFn = ADUC_Result (*)(const ADUC_WorkflowHandle workflowHandle);

DownloadHandlerPlugin::DownloadHandlerPlugin(const std::string& libPath, ADUC_LOG_SEVERITY logLevel)
{
    lib = std::unique_ptr<aduc::SharedLib>(new aduc::SharedLib(libPath));
    lib->EnsureSymbols({ "Initialize", "Cleanup", "ProcessUpdate", "OnUpdateWorkflowCompleted" });

    void* sym = lib->GetSymbol("Initialize");
    auto initializeFn = reinterpret_cast<InitializeFn>(sym);
    initializeFn(logLevel);
}

DownloadHandlerPlugin::~DownloadHandlerPlugin()
{
    void* sym = lib->GetSymbol("Cleanup");
    auto cleanupFn = reinterpret_cast<CleanupFn>(sym);
    cleanupFn();
}

ADUC_Result DownloadHandlerPlugin::ProcessUpdate(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* fileEntity, const char* targetFilePath) const
{
    void* sym = lib->GetSymbol("ProcessUpdate");
    auto produceUpdateFn = reinterpret_cast<ProcessUpdateFn>(sym);
    return produceUpdateFn(workflowHandle, fileEntity, targetFilePath);
}

ADUC_Result DownloadHandlerPlugin::OnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle) const
{
    void* sym = lib->GetSymbol("OnUpdateWorkflowCompleted");
    auto onUpdateWorkflowCompletedFn = reinterpret_cast<OnUpdateWorkflowCompletedFn>(sym);
    return onUpdateWorkflowCompletedFn(workflowHandle);
}

EXTERN_C_BEGIN

ADUC_Result ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted(
    const DownloadHandlerHandle handle, const ADUC_WorkflowHandle workflowHandle)
{
    ADUC_Result result = { ADUC_Result_Failure,
                           ADUC_ERC_DOWNLOAD_HANDLER_PLUGIN_ON_UPDATE_WORKFLOW_COMPLETED_FAILURE };
    try
    {
        std::unique_ptr<DownloadHandlerPlugin> plugin{ reinterpret_cast<DownloadHandlerPlugin*>(handle) };
        result = plugin->OnUpdateWorkflowCompleted(workflowHandle);
    }
    catch (...)
    {
    }
    return result;
}

EXTERN_C_END
