/**
 * @file simulator_handler.cpp
 * @brief Implementation of ContentHandler API for update content simulator.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 */
#include "aduc/simulator_handler.hpp"

#include "aduc/adu_core_exports.h"
#include "aduc/logging.h"
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"
#include "aduc/workflow_utils.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>

#include <dirent.h>

EXTERN_C_BEGIN

/**
 * @brief Instantiates an Update Content Handler for 'microsoft/simulator:1' update type.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "simulator-handler");
    Log_Info("Instantiating an Update Content Handler for 'microsoft/simulator:1'");
    try
    {
        return SimulatorHandlerImpl::CreateContentHandler();
    }
    catch (const std::exception& e)
    {
        const char* what = e.what();
        Log_Error("Unhandled std exception: %s", what);
    }
    catch (...)
    {
        Log_Error("Unhandled exception");
    }

    return nullptr;}

EXTERN_C_END

// Forward declarations.
static ADUC_Result CancelApply(const char* logFolder);

/**
 * @brief Creates a new SimulatorHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a SimulatorHandlerImpl directly.
 *
 * @return ContentHandler* SimulatorHandlerImpl object as a ContentHandler.
 */
ContentHandler* SimulatorHandlerImpl::CreateContentHandler()
{
    return new SimulatorHandlerImpl();
}

/**
 * @brief Mock implementation of download
 * @return ADUC_Result The result (always success)
 */
ADUC_Result SimulatorHandlerImpl::Download(const ADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    Log_Info("Download called - returning success");
    return ADUC_Result{ ADUC_Result_Download_Success };
}

/**
 * @brief Mock implementation of install
 * @return ADUC_Result The result (always success)
 */
ADUC_Result SimulatorHandlerImpl::Install(const ADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    Log_Info("Install called - returning success");
    return ADUC_Result{ ADUC_Result_Install_Success };
}

/**
 * @brief Mock implementation of apply
 * @return ADUC_Result The result (always success)
 */
ADUC_Result SimulatorHandlerImpl::Apply(const ADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    _isInstalled = true;
    Log_Info("Apply called - returning success");
    return ADUC_Result{ ADUC_Result_Apply_Success };
}

/**
 * @brief Mock implementation of cancel
 * @return ADUC_Result The result (always success)
 */
ADUC_Result SimulatorHandlerImpl::Cancel(const ADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    Log_Info("Cancel called - returning success");
    return ADUC_Result{ ADUC_Result_Cancel_Success };
}

void SimulatorHandlerImpl::SetIsInstalled(bool isInstalled)
{
    Log_Info("Setting 'isInstall' to %s", isInstalled);
    _isInstalled = isInstalled;
}

/**
 * @brief Mock implementation of IsInstalled check.
 * @return ADUC_Result The result based on evaluating the installed criteria.
 */
ADUC_Result SimulatorHandlerImpl::IsInstalled(const ADUC_WorkflowData* workflowData)
{
    char* installedCriteria = workflow_get_installed_criteria(workflowData->WorkflowHandle);
    ADUC_Result result;

    if (_isInstalled)
    {
        Log_Info("IsInstalled called - Installed criteria %s was installed.", installedCriteria);
        result = { ADUC_Result_IsInstalled_Installed };
    }

    Log_Info("IsInstalled called - Installed criteria %s was not installed", installedCriteria);
    result = { ADUC_Result_IsInstalled_NotInstalled };

    workflow_free_string(installedCriteria);
    return result;
}