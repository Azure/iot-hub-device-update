/**
 * @file pvcontrol_handler.cpp
 * @brief Implementation of ContentHandler API for pvcontrol.
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
#include "aduc/pvcontrol_handler.hpp"

#include "aduc/adu_core_exports.h"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"
#include "adushell_const.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>

#include <parson.h>
#include <dirent.h>

namespace adushconst = Adu::Shell::Const;

/**
 * @brief handler creation function
 * This function calls  CreateContentHandler from handler factory 
 */
std::unique_ptr<ContentHandler> pantacor_pvcontrol_CreateFunc(const ContentHandlerCreateData& data)
{
    Log_Info("pantacor_pvcontrol_CreateFunc called.");
    return std::unique_ptr<ContentHandler>{ PVControlHandlerImpl::CreateContentHandler(
        data.WorkFolder(), data.LogFolder(), data.Filename()) };
}

/**
 * @brief Creates a new PVControlHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a PVControlHandlerImpl directly.
 *
 * @param workFolder The folder where content will be downloaded.
 * @param logFolder The folder where operational logs can be placed.
 * @param filename The .swu image file to be installed by pvcontrol.
 * @return std::unique_ptr<ContentHandler> SimulatorHandlerImpl object as a ContentHandler.
 */
std::unique_ptr<ContentHandler> PVControlHandlerImpl::CreateContentHandler(
    const std::string& workFolder, const std::string& logFolder, const std::string& filename)
{
    return std::unique_ptr<ContentHandler>{ new PVControlHandlerImpl(workFolder, logFolder, filename) };
}

/**
 * @brief Validate meta data including file count and handler version.
 * @param prepareInfo.
 * @return bool
 */
ADUC_Result PVControlHandlerImpl::Prepare(const ADUC_PrepareInfo* prepareInfo)
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
 * @brief Download implementation for pvcontrol (no-op)
 * pvcontrol does not need to download additional content.
 * This method is a no-op.
 *
 * @return ADUC_Result The result of the download (always success)
 */
ADUC_Result PVControlHandlerImpl::Download()
{
    _isApply = false;
    Log_Info("Download called - no-op for pvcontrol");

    return ADUC_Result{ ADUC_DownloadResult_Success };
}

/**
 * @brief Install implementation for pvcontrol.
 * Calls into the pvcontrol wrapper script to install an image file.
 *
 * @return ADUC_Result The result of the install.
 */
ADUC_Result PVControlHandlerImpl::Install()
{
    _isApply = false;
    Log_Info("Installing from %s", _workFolder.c_str());

    std::unique_ptr<DIR, std::function<int(DIR*)>> directory(
    opendir(_workFolder.c_str()), [](DIR* dirp) -> int { return closedir(dirp); });
    if (directory == nullptr)
    {
        Log_Error("opendir failed, errno = %d", errno);

        return ADUC_Result{ ADUC_InstallResult_Failure, MAKE_ADUC_ERRNO_EXTENDEDRESULTCODE(errno) };
    }

    // Get the tarball path used as input by pvcontrol install
    const char* filename = nullptr;
    const dirent* entry = nullptr;
    while ((entry = readdir(directory.get())) != nullptr)
    {
        if (entry->d_type == DT_REG)
        {
            if (filename == nullptr)
            {
                filename = entry->d_name;
            }
            else
            {
                Log_Error("More than one file in work folder");
                return ADUC_Result{ ADUC_InstallResult_Failure, ADUC_ERC_NOTPERMITTED };
            }
        }
    }

    if (filename == nullptr)
    {
        Log_Error("No file in work folder");
        return ADUC_Result{ ADUC_InstallResult_Failure, ADUC_ERC_NOTRECOVERABLE };
    }

    if (_filename != filename)
    {
        Log_Warn("Specified filename %s does not match actual filename %s.", _filename.c_str(), filename);
    }

    Log_Info("Installing image file: %s", filename);

    // Call the pvcontrol install script
    std::string command = adushconst::adu_shell;
    std::vector<std::string> args{ adushconst::update_type_opt,
                                   adushconst::update_type_pantacor_pvcontrol,
                                   adushconst::update_action_opt,
                                   adushconst::update_action_install };

    std::stringstream data;
    data << _workFolder << "/" << filename;
    args.emplace_back(adushconst::target_data_opt);
    args.emplace_back(data.str().c_str());

    std::string output;
    const int exitCode = ADUC_LaunchChildProcess(command, args, output);

    if (exitCode != 0)
    {
        Log_Error("Install failed, extendedResultCode = %d", exitCode);
        return ADUC_Result{ ADUC_InstallResult_Failure, exitCode };
    }

    Log_Info("Install succeeded");
    return ADUC_Result{ ADUC_InstallResult_Success };
}

/**
 * @brief Apply implementation for pvcontrol.
 * Calls into the pvcontrol wrapper script to perform apply.
 * Will flip bootloader flag to boot into update partition for A/B update.
 *
 * @return ADUC_Result The result of the apply.
 */
ADUC_Result PVControlHandlerImpl::Apply()
{
    _isApply = true;
    Log_Info("Applying data from %s", ADUC_PVINSTALLED_FILE_PATH);

    JSON_Value* progressData = json_parse_file(ADUC_PVINSTALLED_FILE_PATH);
    if (progressData != nullptr)
    {
        const std::string revision = json_object_get_string(json_object(progressData), "revision");

		Log_Info("Applying revision: %s", revision);

        // Call the pvcontrol commands run
        std::string command = adushconst::adu_shell;
        std::vector<std::string> args{ adushconst::update_type_opt,
                                       adushconst::update_type_pantacor_pvcontrol,
                                       adushconst::update_action_opt,
                                       adushconst::update_action_apply };

        args.emplace_back(adushconst::target_data_opt);
        args.emplace_back(revision);

        std::string output;
        const int exitCode = ADUC_LaunchChildProcess(command, args, output);

        if (exitCode != 0)
        {
            Log_Error("Apply failed, extendedResultCode = %d", exitCode);
            return ADUC_Result{ ADUC_ApplyResult_Failure, exitCode };
        }

        Log_Info("Apply succeeded");
        return ADUC_Result{ ADUC_ApplyResult_Success };
    }

    Log_Error("Installed file failure");
    return ADUC_Result{ ADUC_ApplyResult_Failure };
}

/**
 * @brief Cancel implementation for pvcontrol.
 * We don't have many hooks into pvcontrol to cancel an ongoing install.
 * We can cancel apply by reverting the bootloader flag to boot into the original partition.
 * Calls into the pvcontrol wrapper script to cancel apply.
 * Cancel after or during any other operation is a no-op.
 *
 * @return ADUC_Result The result of the cancel.
 */
ADUC_Result PVControlHandlerImpl::Cancel()
{
    if (_isApply)
    {
        return ADUC_Result{ ADUC_CancelResult_Success };
    }

    return ADUC_Result{ ADUC_CancelResult_Success };
}

/**
 * @brief Checks if the installed content matches the installed criteria.
 *
 * @param installedCriteria The installed criteria string. e.g. The firmware version.
 *  installedCriteria has already been checked to be non-empty before this call.
 *
 * @return ADUC_Result
 */
ADUC_Result PVControlHandlerImpl::IsInstalled(const std::string& installedCriteria)
{
    Log_Info("Getting status from version %s", installedCriteria.c_str());

    // Call pvcontrol to get status files updated
    std::string command = adushconst::adu_shell;
    std::vector<std::string> args{ adushconst::update_type_opt,
                                   adushconst::update_type_pantacor_pvcontrol,
                                   adushconst::update_action_opt,
                                   adushconst::update_action_get_status };

    args.emplace_back(adushconst::target_data_opt);
    args.emplace_back(installedCriteria.c_str());

    std::string output;
    const int exitCode = ADUC_LaunchChildProcess(command, args, output);

    if (exitCode != 0)
    {
        Log_Error("Get status failed, extendedResultCode = %d", exitCode);
        return ADUC_Result{ ADUC_IsInstalledResult_Failure, exitCode };
    }

    Log_Info("Checking install status");

    JSON_Value* progressData = json_parse_file(ADUC_PVPROGRESS_FILE_PATH);
    if (progressData != nullptr)
    {
        const std::string status = json_object_get_string(json_object(progressData), "status");
        if ((status == "DONE") || (status == "UPDATED"))
        {
            Log_Info("Update install success");
            return ADUC_Result{ ADUC_IsInstalledResult_Installed };
        }
        if ((status == "ERROR") || (status == "WONTGO"))
        {
            Log_Error("Update install status failure");
            return ADUC_Result{ ADUC_IsInstalledResult_Failure };
        }
        Log_Info("Update install in progress");
        return ADUC_Result{ ADUC_IsInstalledResult_NotInstalled };
    }

    Log_Error("Update install progress file failure");
    return ADUC_Result{ ADUC_IsInstalledResult_Failure };
}
