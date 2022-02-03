/**
 * @file diagnostics_async_helper.h
 * @brief Implementation for the asynchronous DiagnosticsWorkflow upload of logs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "diagnostics_async_helper.h"

#include <aduc/logging.h>
#include <aduc/string_handle_wrapper.hpp>
#include <azure_c_shared_utility/strings.h>
#include <diagnostics_workflow.h>
#include <mutex>
#include <operation_id_utils.h>
#include <thread>
#include <vector>

/**
 * @brief Wraps the DiagnosticsWorkflow to work asynchronously
 * @details Only allows one DiagnosticsWorkflow at a time, will block until the old thread has finished otherwise
 */
class DiagnosticsWorkflowManager
{
private:
    std::thread worker; //!< current thread doing work

public:
    explicit DiagnosticsWorkflowManager() = default;
    DiagnosticsWorkflowManager(const DiagnosticsWorkflowManager&) = delete;
    DiagnosticsWorkflowManager(DiagnosticsWorkflowManager&&) = delete;
    DiagnosticsWorkflowManager& operator=(const DiagnosticsWorkflowManager&) = delete;
    DiagnosticsWorkflowManager& operator=(DiagnosticsWorkflowManager&&) = delete;

    /**
     *@brief Joins the old worker thread if it is still working and then starts a new diagnostics workflow using the parameters passed
     * @param diagnosticsWorkflowData workflowData struct that describes the configuration for the DiagnosticsWorkflow
     * @param jsonStringHandle the message from the PnP interface to be parsed for the operation-id and sas-credential
     */
    void
    StartDiagnosticsWorkflow(const DiagnosticsWorkflowData* diagnosticsWorkflowData, STRING_HANDLE jsonStringHandle)
    {
        try
        {
            if (worker.joinable())
            {
                worker.join();
            }

            //
            // Required to prevent duplicate requests coming down from the service after
            // restart or a connection refresh
            //
            if (OperationIdUtils_OperationIsComplete(STRING_c_str(jsonStringHandle)))
            {
                return;
            }

            STRING_HANDLE jsonStringHandleClone = STRING_clone(jsonStringHandle);

            std::thread newWorker{ [diagnosticsWorkflowData, jsonStringHandleClone] {
                ADUC::StringUtils::STRING_HANDLE_wrapper cloneWrapper(jsonStringHandleClone);

                try
                {
                    DiagnosticsWorkflow_DiscoverAndUploadLogs(diagnosticsWorkflowData, cloneWrapper.c_str());
                }
                catch (const std::exception& e)
                {
                    Log_Error("StartNewDiagnosticsWorkflowThread worker thread failed with exception: %s", e.what());
                }
                catch (...)
                {
                    Log_Error("StartNewDiagnosticsWorkflowThread worker thread failed with unknown exception");
                }
            } };

            worker = std::move(newWorker);
        }
        catch (const std::exception& e)
        {
            Log_Error("StartNewDiagnosticsWorkflowThread worker thread failed with exception: %s", e.what());
        }
        catch (...)
        {
            Log_Error("StartNewDiagnosticsWorkflowThread failed with unknown exception");
        }

        STRING_delete(jsonStringHandle);
    }

    /**
     * @brief Destructor assures that the worker thread will have been joined before exiting
    */
    ~DiagnosticsWorkflowManager()
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
};

static DiagnosticsWorkflowManager s_DiagnosticsManager;

/**
 * @brief Asynchronously begins the Diagnosticss workflow for discovering and uploading logs
 * @details Only asynchronous when there is not already a thread running a workflow.
 * @param[in] workflowData struct containing the configuration information for the diagnostics component
 * @param[in] jsonString json string from the service contianing the operation-id and sas url
 */
void DiagnosticsWorkflow_DiscoverAndUploadLogsAsync(
    const DiagnosticsWorkflowData* workflowData, const char* jsonString)
{
    try
    {
        STRING_HANDLE jsonStringHandle = STRING_construct(jsonString);

        s_DiagnosticsManager.StartDiagnosticsWorkflow(workflowData, jsonStringHandle);
    }
    catch (const std::exception& e)
    {
        Log_Error("DiagnosticsAsyncHelper_DiscoverAndUploadFiles failed with exception: %s", e.what());
    }

    catch (...)
    {
        Log_Error("DiagnosticsAsyncHelper_DiscoverAndUploadFiles failed with unknown exception");
    }
}
