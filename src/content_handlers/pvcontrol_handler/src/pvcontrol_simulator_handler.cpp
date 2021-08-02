/**
 * @file pvcontrol_simulator_handler.cpp
 * @brief Implementation of ContentHandler API for pvcontrol simulator.
 *
 * Will call into wrapper script for pvcontrol to install image files.
 *
 * pantacor/pvcontrol
 * v1:
 *   Description:
 *   Initial revision.
 *
 *   Expected files:
 *   .swu - contains pvcontrol image.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 * @copyright Copyright (c) 2021, Pantacor Ltd.
 */
#include "aduc/pvcontrol_simulator_handler.hpp"

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
std::unique_ptr<ContentHandler> pantacor_pvcontrol_Simulator_CreateFunc(const ContentHandlerCreateData& data)
{
    Log_Info("pantacor_pvcontrol_simulator_CreateFunc called.");
    return std::unique_ptr<ContentHandler>{ PVControlSimulatorHandlerImpl::CreateContentHandler(
        data.WorkFolder(), data.LogFolder(), data.Filename()) };
}

// Forward declarations.
static ADUC_Result CancelApply(const char* logFolder);

/**
 * @brief Creates a new PVControlSimulatorHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a PVControlSimulatorHandlerImpl directly.
 *
 * @param workFolder The folder where content will be downloaded.
 * @param logFolder The folder where operational logs can be placed.
 * @param filename The .swu image file to be installed by pvcontrol.
 * @return std::unique_ptr<ContentHandler> PVControlSimulatorHandlerImpl object as a ContentHandler.
 */
std::unique_ptr<ContentHandler> PVControlSimulatorHandlerImpl::CreateContentHandler(
    const std::string& workFolder, const std::string& logFolder, const std::string& filename)
{
    return std::unique_ptr<ContentHandler>{ new PVControlSimulatorHandlerImpl(workFolder, logFolder, filename) };
}

/**
 * @brief Mock implementation of prepare.
 * @param prepareInfo.
 * @return bool
 */
ADUC_Result PVControlSimulatorHandlerImpl::Prepare(const ADUC_PrepareInfo* prepareInfo)
{
    if (prepareInfo->updateTypeVersion != 1)
    {
        Log_Error("PVControl packages prepare failed. Wrong Handler Version %d", prepareInfo->updateTypeVersion);
        return ADUC_Result{ ADUC_PrepareResult_Failure,
                            ADUC_ERC_PVCONTROL_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_VERSION };
    }

    if (prepareInfo->fileCount != 1)
    {
        Log_Error("PVControl packages prepare failed. Wrong File Count %d", prepareInfo->fileCount);
        return ADUC_Result{ ADUC_PrepareResult_Failure,
                            ADUC_ERC_PVCONTROL_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT };
    }

    Log_Info("Prepare succeeded.");
    return ADUC_Result{ ADUC_PrepareResult_Success };
}

/**
 * @brief Mock implementation of download
 * @return ADUC_Result The result (always success)
 */
ADUC_Result PVControlSimulatorHandlerImpl::Download()
{
    Log_Info("Download called - returning success");
    return ADUC_Result{ ADUC_DownloadResult_Success };
}

/**
 * @brief Mock implementation of install
 * @return ADUC_Result The result (always success)
 */
ADUC_Result PVControlSimulatorHandlerImpl::Install()
{
    Log_Info("Install called - returning success");
    return ADUC_Result{ ADUC_InstallResult_Success };
}

/**
 * @brief Mock implementation of apply
 * @return ADUC_Result The result (always success)
 */
ADUC_Result PVControlSimulatorHandlerImpl::Apply()
{
    _isInstalled = true;
    Log_Info("Apply called - returning success");
    return ADUC_Result{ ADUC_ApplyResult_Success };
}

/**
 * @brief Mock implementation of cancel
 * @return ADUC_Result The result (always success)
 */
ADUC_Result PVControlSimulatorHandlerImpl::Cancel()
{
    Log_Info("Cancel called - returning success");
    return ADUC_Result{ ADUC_CancelResult_Success };
}

/**
 * @brief Mock implementation of IsInstalled check.
 * @return ADUC_Result The result based on evaluating the installed criteria.
 * Note that IsInstalled for pvcontrol simulator will be true if Apply was called.
 */
ADUC_Result PVControlSimulatorHandlerImpl::IsInstalled(const std::string& installedCriteria)
{
    if (_isInstalled)
    {
        Log_Info("IsInstalled called - Installed criteria %s was installed.", installedCriteria.c_str());
        return ADUC_Result{ ADUC_IsInstalledResult_Installed };
    }

    Log_Info("IsInstalled called - Installed criteria %s was not installed", installedCriteria.c_str());
    return ADUC_Result{ ADUC_IsInstalledResult_NotInstalled };
}
