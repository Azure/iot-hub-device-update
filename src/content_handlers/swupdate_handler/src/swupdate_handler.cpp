/**
 * @file swupdate_handler.cpp
 * @brief Implementation of ContentHandler API for swupdate.
 *
 * Will call into wrapper script for swupdate to install image files.
 *
 * microsoft/swupdate
 * v1:
 *   Description:
 *   Initial revision.
 *
 *   Expected files:
 *   .swu - contains swupdate image.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#include "aduc/swupdate_handler.hpp"

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

#include <dirent.h>

namespace adushconst = Adu::Shell::Const;

/**
 * @brief handler creation function
 * This function calls  CreateContentHandler from handler factory 
 */
std::unique_ptr<ContentHandler> microsoft_swupdate_CreateFunc(const ContentHandlerCreateData& data)
{
    Log_Info("microsoft_swupdate_CreateFunc called.");
    return std::unique_ptr<ContentHandler>{ SWUpdateHandlerImpl::CreateContentHandler(
        data.WorkFolder(), data.LogFolder(), data.Filename()) };
}

// Forward declarations.
static ADUC_Result CancelApply(const char* logFolder);

/**
 * @brief Creates a new SWUpdateHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a SWUpdateHandlerImpl directly.
 *
 * @param workFolder The folder where content will be downloaded.
 * @param logFolder The folder where operational logs can be placed.
 * @param filename The .swu image file to be installed by swupdate.
 * @return std::unique_ptr<ContentHandler> SimulatorHandlerImpl object as a ContentHandler.
 */
std::unique_ptr<ContentHandler> SWUpdateHandlerImpl::CreateContentHandler(
    const std::string& workFolder, const std::string& logFolder, const std::string& filename)
{
    return std::unique_ptr<ContentHandler>{ new SWUpdateHandlerImpl(workFolder, logFolder, filename) };
}

/**
 * @brief Validate meta data including file count and handler version.
 * @param prepareInfo.
 * @return bool
 */
ADUC_Result SWUpdateHandlerImpl::Prepare(const ADUC_PrepareInfo* prepareInfo)
{
    if (prepareInfo->updateTypeVersion != 1)
    {
        Log_Error("SWUpdate packages prepare failed. Wrong Handler Version %d", prepareInfo->updateTypeVersion);
        return ADUC_Result{ ADUC_PrepareResult_Failure,
                            ADUC_ERC_SWUPDATE_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_VERSION };
    }

    if (prepareInfo->fileCount != 1)
    {
        Log_Error("SWUpdate packages prepare failed. Wrong File Count %d", prepareInfo->fileCount);
        return ADUC_Result{ ADUC_PrepareResult_Failure,
                            ADUC_ERC_SWUPDATE_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT };
    }

    Log_Info("Prepare succeeded.");
    return ADUC_Result{ ADUC_PrepareResult_Success };
}

/**
 * @brief Download implementation for swupdate (no-op)
 * swupdate does not need to download additional content.
 * This method is a no-op.
 *
 * @return ADUC_Result The result of the download (always success)
 */
ADUC_Result SWUpdateHandlerImpl::Download()
{
    _isApply = false;
    Log_Info("Download called - no-op for swupdate");
    return ADUC_Result{ ADUC_DownloadResult_Success };
}

/**
 * @brief Install implementation for swupdate.
 * Calls into the swupdate wrapper script to install an image file.
 *
 * @return ADUC_Result The result of the install.
 */
ADUC_Result SWUpdateHandlerImpl::Install()
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

    // Calling the install script to install the update. The script takes the name of the image file as input.
    // Only one file is expected in the work folder. If multiple files exist, treat it as a failure.
    // TODO(Nox): Forcing updates to be a single file is a temporary workaround.
    // We need to have the ability to determine which file is the image file.
    // Then we can pass the appropriate file to the install script.
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

    // Execute the install command with  "-i <image_file>"  to install the update image file.
    // For swupdate the image file is typically a .swu file

    // This is equivalent to: command << c_installScript << " -l " << _logFolder << " -i '" << _workFolder << "/" << filename << "'"

    std::string command = adushconst::adu_shell;
    std::vector<std::string> args{ adushconst::update_type_opt,
                                   adushconst::update_type_microsoft_swupdate,
                                   adushconst::update_action_opt,
                                   adushconst::update_action_install };

    std::stringstream data;
    data << _workFolder << "/" << filename;
    args.emplace_back(adushconst::target_data_opt);
    args.emplace_back(data.str().c_str());

    args.emplace_back(adushconst::target_log_folder_opt);
    args.emplace_back(_logFolder);

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
 * @brief Apply implementation for swupdate.
 * Calls into the swupdate wrapper script to perform apply.
 * Will flip bootloader flag to boot into update partition for A/B update.
 *
 * @return ADUC_Result The result of the apply.
 */
ADUC_Result SWUpdateHandlerImpl::Apply()
{
    _isApply = true;
    Log_Info("Applying data from %s", _workFolder.c_str());

    // Execute the install command with  "-a" to apply the install by telling
    // the bootloader to boot to the updated partition.

    // This is equivalent to : command << c_installScript << " -l " << _logFolder << " -a"

    std::string command = adushconst::adu_shell;
    std::vector<std::string> args{ adushconst::update_type_opt,
                                   adushconst::update_type_microsoft_swupdate,
                                   adushconst::update_action_opt,
                                   adushconst::update_action_apply };

    args.emplace_back(adushconst::target_log_folder_opt);
    args.emplace_back(_logFolder);

    std::string output;

    const int exitCode = ADUC_LaunchChildProcess(command, args, output);

    if (exitCode != 0)
    {
        Log_Error("Apply failed, extendedResultCode = %d", exitCode);
        return ADUC_Result{ ADUC_ApplyResult_Failure, exitCode };
    }

    // Always require a reboot after successful apply
    return ADUC_Result{ ADUC_ApplyResult_SuccessRebootRequired };
}

/**
 * @brief Cancel implementation for swupdate.
 * We don't have many hooks into swupdate to cancel an ongoing install.
 * We can cancel apply by reverting the bootloader flag to boot into the original partition.
 * Calls into the swupdate wrapper script to cancel apply.
 * Cancel after or during any other operation is a no-op.
 *
 * @return ADUC_Result The result of the cancel.
 */
ADUC_Result SWUpdateHandlerImpl::Cancel()
{
    if (_isApply)
    {
        // swupdate handler can only cancel apply.
        // all other cancels are no-ops.
        return CancelApply(_logFolder.c_str());
    }

    return ADUC_Result{ ADUC_CancelResult_Success };
}

/**
 * @brief Reads a first line of a file, trims trailing whitespace, and returns as string.
 *
 * @param filePath Path to the file to read value from.
 * @return std::string Returns the value from the file. Returns empty string if there was an error.
 */
/*static*/
std::string SWUpdateHandlerImpl::ReadValueFromFile(const std::string& filePath)
{
    if (filePath.empty())
    {
        Log_Error("Empty file path.");
        return std::string{};
    }

    if ((filePath.length()) + 1 > PATH_MAX)
    {
        Log_Error("Path is too long.");
        return std::string{};
    }

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        Log_Error("File %s failed to open, error: %d", filePath.c_str(), errno);
        return std::string{};
    }

    std::string result;
    std::getline(file, result);
    if (file.bad())
    {
        Log_Error("Unable to read from file %s, error: %d", filePath.c_str(), errno);
        return std::string{};
    }

    // Trim whitespace
    ADUC::StringUtils::Trim(result);
    return result;
}

/**
 * @brief Checks if the installed content matches the installed criteria.
 *
 * @param installedCriteria The installed criteria string. e.g. The firmware version.
 *  installedCriteria has already been checked to be non-empty before this call.
 *
 * @return ADUC_Result
 */
ADUC_Result SWUpdateHandlerImpl::IsInstalled(const std::string& installedCriteria)
{
    std::string version{ ReadValueFromFile(ADUC_VERSION_FILE) };
    if (version.empty())
    {
        Log_Error("Version file %s did not contain a version or could not be read.", ADUC_VERSION_FILE);
        return ADUC_Result{ ADUC_IsInstalledResult_Failure };
    }

    if (version == installedCriteria)
    {
        Log_Info("Installed criteria %s was installed.", installedCriteria.c_str());
        return ADUC_Result{ ADUC_IsInstalledResult_Installed };
    }

    Log_Info(
        "Installed criteria %s was not installed, the current version is %s",
        installedCriteria.c_str(),
        version.c_str());
    return ADUC_Result{ ADUC_IsInstalledResult_NotInstalled };
}

/**
 * @brief Helper function to perform cancel when we are doing an apply.
 *
 * @return ADUC_Result The result of the cancel.
 */
static ADUC_Result CancelApply(const char* logFolder)
{
    // Execute the install command with  "-r" to reverts the apply by
    // telling the bootloader to boot into the current partition

    // This is equivalent to : command << c_installScript << " -l " << logFolder << " -r"

    std::string command = adushconst::adu_shell;
    std::vector<std::string> args{ adushconst::update_type_opt,       adushconst::update_type_microsoft_swupdate,
                                   adushconst::update_action_opt,     adushconst::update_action_apply,
                                   adushconst::target_log_folder_opt, logFolder };

    std::string output;

    const int exitCode = ADUC_LaunchChildProcess(command, args, output);
    if (exitCode != 0)
    {
        // If failed to cancel apply, apply should return SuccessRebootRequired.
        Log_Error("Failed to cancel Apply, extendedResultCode = %d", exitCode);
        return ADUC_Result{ ADUC_CancelResult_Failure, exitCode };
    }

    Log_Info("Apply was cancelled");
    return ADUC_Result{ ADUC_ApplyResult_Cancelled };
}