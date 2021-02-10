/**
 * @file apt_simulator_handler.cpp
 * @brief Implementation of ContentHandler API for apt simulator.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#include "aduc/apt_simulator_handler.hpp"

#include <aduc/adu_core_exports.h>
#include <aduc/logging.h>
#include <aduc/string_utils.hpp>
#include <aduc/system_utils.h>

#include <algorithm>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>

#include <dirent.h>

/**
 * @brief handler creation function
 * This function calls  CreateContentHandler from handler factory 
 */
std::unique_ptr<ContentHandler> microsoft_apt_Simulator_CreateFunc(const ContentHandlerCreateData& data)
{
    Log_Info("microsoft_apt_simulator_CreateFunc called.");
    return std::unique_ptr<ContentHandler>{ AptSimulatorHandlerImpl::CreateContentHandler(
        data.WorkFolder(),  data.Filename()) };
}

/**
 * @brief Mock implementation of download
 * @param PrepareInfo.
 * @return ADUC_Result the result of prepare
 */
ADUC_Result AptSimulatorHandlerImpl::Prepare(const ADUC_PrepareInfo* prepareInfo)
{
    Log_Info("Prepare succeeded.");
    return ADUC_Result{ ADUC_PrepareResult_Success };
}

// Forward declarations.
static ADUC_Result CancelApply(const char* logFolder);

/**
 * @brief Creates a new AptSimulatorHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a AptSimulatorHandlerImpl directly.
 *
 * @param workFolder The folder where content will be downloaded.
 * @param filename The APT manifest file.
 * @return std::unique_ptr<ContentHandler> AptSimulatorHandlerImpl object as a ContentHandler.
 */
std::unique_ptr<ContentHandler> AptSimulatorHandlerImpl::CreateContentHandler(
    const std::string& workFolder, const std::string& filename)
{
    return std::unique_ptr<ContentHandler>{ new AptSimulatorHandlerImpl(workFolder, filename) };
}

/**
 * @brief Mock implementation of download
 * @return ADUC_Result The result (always success)
 */
ADUC_Result AptSimulatorHandlerImpl::Download()
{
    Log_Info("Download called - returning success");
    return ADUC_Result{ ADUC_DownloadResult_Success };
}

/**
 * @brief Mock implementation of install
 * @return ADUC_Result The result (always success)
 */
ADUC_Result AptSimulatorHandlerImpl::Install()
{
    Log_Info("Install called - returning success");
    return ADUC_Result{ ADUC_InstallResult_Success };
}

/**
 * @brief Mock implementation of apply
 * @return ADUC_Result The result (always success)
 */
ADUC_Result AptSimulatorHandlerImpl::Apply()
{
    _isInstalled = true;
    Log_Info("Apply called - returning success");
    return ADUC_Result{ ADUC_ApplyResult_Success };
}

/**
 * @brief Mock implementation of cancel
 * @return ADUC_Result The result (always success)
 */
ADUC_Result AptSimulatorHandlerImpl::Cancel()
{
    Log_Info("Cancel called - returning success");
    return ADUC_Result{ ADUC_CancelResult_Success };
}

/**
 * @brief Mock implementation of IsInstalled check.
 * @return ADUC_Result The result based on evaluating the installed criteria.
 */
ADUC_Result AptSimulatorHandlerImpl::IsInstalled(const std::string& installedCriteria)
{
    if (_isInstalled)
    {
        Log_Info("IsInstalled called - Installed criteria %s was installed.", installedCriteria.c_str());
        return ADUC_Result{ ADUC_IsInstalledResult_Installed };
    }

    Log_Info("IsInstalled called - Installed criteria %s was not installed", installedCriteria.c_str());
    return ADUC_Result{ ADUC_IsInstalledResult_NotInstalled };
}