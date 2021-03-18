/**
 * @file apt_handler.cpp
 * @brief Implementation of APT handler plug-in for APT (Advanced Package Tool)
 *
 * microsoft/apt
 * v1:
 *   Description:
 *   Initial revision.
 *
 *   Expected files:
 *   <manifest>.json - contains apt configuration and package list.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#include "aduc/apt_handler.hpp"
#include "aduc/adu_core_exports.h"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "adushell_const.hpp"

#include <chrono>
#include <fstream>
#include <parson.h>
#include <sstream>

#ifdef ADUC_SIMULATOR_MODE
#    error Simulator for APT update type is yet not supported.
#endif

namespace adushconst = Adu::Shell::Const;

/**
 * @brief Constructor
 * This function locates and parses APT file using APTParser.
 */
std::unique_ptr<ContentHandler> microsoft_apt_CreateFunc(const ContentHandlerCreateData& data)
{
    Log_Info("microsoft_apt_CreateFunc called.");
    return std::unique_ptr<ContentHandler>{ AptHandlerImpl::CreateContentHandler(data.WorkFolder(), data.Filename()) };
}

/**
 * @brief Constructor
 * This function locates and parses APT file using APTParser.
 */
AptHandlerImpl::AptHandlerImpl(const std::string& workFolder, const std::string& filename) :
    _workFolder(workFolder), _filename(filename)
{
    _applied = false;

    if (workFolder.empty() || filename.empty())
    {
        return;
    }

    // Parse the APT file.
    try
    {
        _aptContent = AptParser::ParseAptContentFromFile(_workFolder + "/" + _filename);
    }
    catch (const AptParser::ParserException& pe)
    {
        _aptContent = nullptr;
        Log_Error("An error occurred while parsing APT. %s", pe.what());
    }

    if (_aptContent)
    {
        _packages = _aptContent->Packages;

        // create apt content Id
        CreatePersistedId();
    }
    else
    {
        throw AptHandlerException(
            "An error occurred while initalizing APT content", ADUC_ERC_APT_HANDLER_INITIALIZATION_FAILURE);
    }
}

/**
 * @brief Creates a new APTHandlerImpl object and casts to a ContentHandler.
 * @details Note that there is no way to create a APTHandlerImpl directly.
 *
 * @param workFolder The folder where content will be downloaded.
 * @param filename The .json file contain the APT data.
 * @return std::unique_ptr<ContentHandler> APTHandlerImpl object as a ContentHandler.
 */
std::unique_ptr<ContentHandler>
AptHandlerImpl::CreateContentHandler(const std::string& workFolder, const std::string& filename)
{
    return std::unique_ptr<ContentHandler>{ new AptHandlerImpl(workFolder, filename) };
}

/**
 * @brief Validate meta data including file count and handler version.
 * @param PrepareInfo.
 * @return ADUC_Result the result of prepare
 */
ADUC_Result AptHandlerImpl::Prepare(const ADUC_PrepareInfo* prepareInfo)
{
    if (prepareInfo->updateTypeVersion != 1)
    {
        Log_Error("APT packages prepare failed. Wrong Handler Version %d", prepareInfo->updateTypeVersion);
        return ADUC_Result{ ADUC_PrepareResult_Failure, ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_VERSION };
    }

    if (prepareInfo->fileCount != 1)
    {
        Log_Error("APT packages prepare failed. Wrong File Count %d", prepareInfo->fileCount);
        return ADUC_Result{ ADUC_PrepareResult_Failure, ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT };
    }

    Log_Info("Prepare succeeded.");
    return ADUC_Result{ ADUC_PrepareResult_Success };
}

/**
 * @brief Download implementation for APT handler.
 *
 * @return ADUC_Result The result of the download.
 */
ADUC_Result AptHandlerImpl::Download()
{
    _applied = false;

    std::string aptOutput;
    int aptExitCode = -1;

    // Perform apt-get update to fetch latest packages catalog.
    // We'll log warning if failed, but will try to download specified packages.
    try
    {
        // Perform apt-get update.
        const std::vector<std::string> args = { adushconst::update_type_opt,
                                                adushconst::update_type_microsoft_apt,
                                                adushconst::update_action_opt,
                                                adushconst::update_action_initialize };

        aptExitCode = ADUC_LaunchChildProcess(adushconst::adu_shell, args, aptOutput);

        if (!aptOutput.empty())
        {
            Log_Info(aptOutput.c_str());
        }
    }
    catch (const std::exception& de)
    {
        Log_Error("Exception occurred while processing apt-get update.\n%s", de.what());
        aptExitCode = -1;
    }

    if (aptExitCode != 0)
    {
        Log_Error("APT update failed. (Exit code: %d)", aptExitCode);
    }

    // Download packages.
    ADUC_Result result = { ADUC_DownloadResult_Success };
    try
    {
        std::vector<std::string> args = { adushconst::update_type_opt,
                                          adushconst::update_type_microsoft_apt,
                                          adushconst::update_action_opt,
                                          adushconst::update_action_download };

        // For microsoft/apt, target-data is a list of packages.
        std::stringstream data;
        data << "'";
        for (const std::string& package : _packages)
        {
            data << package << " ";
        }
        data << "'";

        args.emplace_back(adushconst::target_data_opt);
        args.emplace_back(data.str());

        aptExitCode = ADUC_LaunchChildProcess(adushconst::adu_shell, args, aptOutput);

        if (!aptOutput.empty())
        {
            Log_Info(aptOutput.c_str());
        }
    }
    catch (const std::exception& de)
    {
        Log_Error("Exception occurred during download. %s", de.what());
        aptExitCode = -1;
    }

    if (aptExitCode != 0)
    {
        result.ResultCode = ADUC_DownloadResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_APT_HANDLER_PACKAGE_DOWNLOAD_FAILURE;
        Log_Error("APT packages download failed. (Exit code: %d)", aptExitCode);
    }

    return result;
}

/**
 * @brief Install implementation for APT Handler.
 *
 * @return ADUC_Result The result of the install.
 */
ADUC_Result AptHandlerImpl::Install()
{
    _applied = false;
    std::string aptOutput;
    int aptExitCode = -1;

    ADUC_Result result = { ADUC_DownloadResult_Success };
    try
    {
        std::vector<std::string> args = { adushconst::update_type_opt,
                                          adushconst::update_type_microsoft_apt,
                                          adushconst::update_action_opt,
                                          adushconst::update_action_install };

        //
        // For public preview, we're passing following additional options to apt-get.
        //
        // '-y' (assumed yes)
        //  -o Dpkg::Options::=--force-confdef -o Dpkg::Options::=--force-confold" (preserve existing config.yaml file by default)
        //
        args.emplace_back(adushconst::target_options_opt);
        args.emplace_back("-o Dpkg::Options::=--force-confdef -o Dpkg::Options::=--force-confold");

        // For microsoft/apt, target-data is a list of packages.
        std::stringstream data;
        for (const std::string& package : _packages)
        {
            data << package << " ";
        }
        args.emplace_back(adushconst::target_data_opt);
        args.emplace_back(data.str());

        aptExitCode = ADUC_LaunchChildProcess(adushconst::adu_shell, args, aptOutput);

        if (!aptOutput.empty())
        {
            Log_Info(aptOutput.c_str());
        }
    }
    catch (const std::exception& de)
    {
        Log_Error("Exception occurred during install. %s", de.what());
        aptExitCode = -1;
    }

    if (aptExitCode != 0)
    {
        result.ResultCode = ADUC_InstallResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_APT_HANDLER_PACKAGE_INSTALL_FAILURE;
        Log_Error("APT packages install failed. (Exit code: %d)", aptExitCode);
    }

    return result;
}

/**
 * @brief Apply implementation for APT Handler.
 *
 * @return ADUC_Result The result of the apply.
 */
ADUC_Result AptHandlerImpl::Apply()
{
    ADUC_Result result = { ADUC_ApplyResult_Success };
    _applied = true;
    if (!PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, _aptContent->Id))
    {
        return ADUC_Result{ ADUC_ApplyResult_Failure, ADUC_ERC_APT_HANDLER_INSTALLCRITERIA_PERSIST_FAILURE };
    }

    if (_aptContent->AgentRestartRequired)
    {
        Log_Debug("The install task completed successfully, DU Agent restart is required for this update.");
        return ADUC_Result{ ADUC_ApplyResult_SuccessAgentRestartRequired };
    }

    Log_Info("Apply succeeded");
    return result;
}

/**
 * @brief Cancel implementation for APT Handler.
 *
 * @return ADUC_Result The result of the cancel.
 */
ADUC_Result AptHandlerImpl::Cancel()
{
    // For private preview, do nothing for now.
    if (_applied)
    {
        return ADUC_Result{ ADUC_CancelResult_Failure, ADUC_ERC_APT_HANDLER_PACKAGE_CANCEL_FAILURE };
    }

    return ADUC_Result{ ADUC_CancelResult_Success };
}

/**
 * @brief Checks if the installed content matches the installed criteria.
 *
 * @param installedCriteria The installed criteria string. e.g. The firmware version or APT id.
 *  installedCriteria has already been checked to be non-empty before this call.
 *
 * @return ADUC_Result
 */
ADUC_Result AptHandlerImpl::IsInstalled(const std::string& installedCriteria)
{
    return GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria);
}

/**
 * @brief Checks if the installed content matches the installed criteria.
 *
 * @param installedCriteria The installed criteria string. e.g. The firmware version or APT id.
 *  installedCriteria has already been checked to be non-empty before this call.
 *
 * @return ADUC_Result
 */
const ADUC_Result
AptHandlerImpl::GetIsInstalled(const char* installedCriteriaFilePath, const std::string& installedCriteria)
{
    // For any error, we'll return 'Not Installed'.
    ADUC_Result result = ADUC_Result{ ADUC_IsInstalledResult_NotInstalled };
    Log_Info("Evaluating installedCriteria %s", installedCriteria.c_str());

    JSON_Value* rootValue = json_parse_file(installedCriteriaFilePath);
    if (rootValue != nullptr)
    {
        JSON_Array* icArray = json_value_get_array(rootValue);
        bool foundInstalledCriteria = false;
        for (size_t i = 0; i < json_array_get_count(icArray); i++)
        {
            JSON_Object* icObject = json_array_get_object(icArray, i);
            if (icObject != nullptr)
            {
                const char* criteria = json_object_get_string(icObject, "installedCriteria");
                const std::string state = json_object_get_string(icObject, "state");

                Log_Debug("Found installedCriteria: %s, state:%s ", criteria, state.c_str());
                if ((criteria != nullptr) && (installedCriteria == criteria))
                {
                    foundInstalledCriteria = true;
                    result = ADUC_Result{ state == "installed" ? ADUC_IsInstalledResult_Installed
                                                               : ADUC_IsInstalledResult_NotInstalled };
                    if (result.ResultCode == ADUC_IsInstalledResult_NotInstalled)
                    {
                        Log_Info(
                            "Installed criteria %s is found, but the state is %s, not Installed",
                            installedCriteria.c_str(),
                            state.c_str());
                    }
                    break;
                }
            }
        }
        if (!foundInstalledCriteria)
        {
            Log_Info("Installed criteria %s is not found in the list of packages.", installedCriteria.c_str());
        }

        json_value_free(rootValue);
    }

    return result;
}

/**
 * @brief Serialize specified JSON_Value and atomically save to specified file.
 * Note that this function will write serialized data to a temp file, then rename (or replace the existing file)
 * the temp file to specified 'filename'.
 *
 * The temp filename is generated by appending an epoch time to specified 'filepath' param).
 *
 * @param value A JSON_Value to be serialized.
 * @param filepath A fully-qualified path to the output file.
 *
 * @return JSON_Status A value indicates whether serialization and file operations are succeeded.
 */
const JSON_Status safe_json_serialize_to_file_pretty(const JSON_Value* value, const char* filepath)
{
    std::string tempFilepath = filepath;
    tempFilepath += std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

    JSON_Status status = json_serialize_to_file_pretty(value, tempFilepath.c_str());
    if (status == JSONSuccess)
    {
        if (rename(tempFilepath.c_str(), filepath) != 0)
        {
            remove(tempFilepath.c_str());
            status = JSONFailure;
        }
    }

    return status;
}

/**
 * @brief Persist specified installedCriteria in a file and mark its state as 'installed'.
 * NOTE: For private preview, only the last installedCriteria is persisted.
 * 
 * @param installedCriteriaFilePath A full path to installed criteria data file.
 * @param installedCriteria An installed criteria string.
 *
 * @return bool A boolean indicates whether installedCriteria added successfully.
 */
const bool
AptHandlerImpl::PersistInstalledCriteria(const char* installedCriteriaFilePath, const std::string& installedCriteria)
{
    Log_Debug("Saving installedCriteria: %s ", installedCriteria.c_str());

    bool success = false;
    JSON_Status status;

    JSON_Value* rootValue = json_parse_file(installedCriteriaFilePath);

    if (rootValue == nullptr)
    {
        rootValue = json_value_init_array();
    }

    if (rootValue != nullptr)
    {
        JSON_Array* rootArray = json_value_get_array(rootValue);
        JSON_Value* icValue = json_value_init_object();
        JSON_Object* icObject = json_value_get_object(icValue);

        if (JSONSuccess == (status = json_object_set_string(icObject, "installedCriteria", installedCriteria.c_str())))
        {
            std::chrono::system_clock::duration timeSinceEpoch = std::chrono::system_clock::now().time_since_epoch();
            const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeSinceEpoch).count();

            if (JSONSuccess == (status = json_object_set_string(icObject, "state", "installed")))
            {
                if (JSONSuccess == (status = json_object_set_number(icObject, "timestamp", seconds)))
                {
                    if (JSONSuccess == (status = json_array_append_value(rootArray, icValue)))
                    {
                        status = safe_json_serialize_to_file_pretty(rootValue, installedCriteriaFilePath);
                    }
                }
            }
        }

        success = (status == JSONSuccess);
        json_value_free(rootValue);
    }

    return success;
}

/**
 * @brief Remove specified installedCriteria from installcriteria data file.
 *
 * @param installedCriteriaFilePath A full path to installed criteria data file.
 * @param installedCriteria An installed criteria string.
 *
 * @return bool 'True' if the specified installedCriteria doesn't exist, file doesn't exist, or removed successfully.
 */
const bool
AptHandlerImpl::RemoveInstalledCriteria(const char* installedCriteriaFilePath, const std::string& installedCriteria)
{
    bool success = true;
    JSON_Status status;

    std::ifstream dataFile(installedCriteriaFilePath);
    if (!dataFile.good())
    {
        // File doesn't exist.
        return true;
    }

    JSON_Value* rootValue = json_parse_file(installedCriteriaFilePath);
    if (rootValue != nullptr)
    {
        JSON_Array* icArray = json_value_get_array(rootValue);
        for (size_t i = json_array_get_count(icArray); i > 0; i--)
        {
            JSON_Object* ic = json_array_get_object(icArray, i - 1);
            if (ic != nullptr)
            {
                const char* id = json_object_get_string(ic, "installedCriteria");
                if (installedCriteria == id)
                {
                    JSON_Status deleted = json_array_remove(icArray, i - 1);
                    if (deleted == JSONSuccess)
                    {
                        JSON_Status saved = safe_json_serialize_to_file_pretty(rootValue, installedCriteriaFilePath);
                        success = (saved == JSONSuccess);
                    }
                    else
                    {
                        // Failed to remove.
                        success = false;
                    }
                    break;
                }
            }
        }

        json_value_free(rootValue);
    }
    else
    {
        success = false;
    }

    return success;
}

void AptHandlerImpl::RemoveAllInstalledCriteria()
{
    remove(ADUC_INSTALLEDCRITERIA_FILE_PATH);
}

/**
 * @brief Set_aptContent->Id with format "<name>-<version>""
 * This id will be the persisted installCriteria saved into a file and marked as installed
 * The persisted installCriteria will be checked in the IsInstalled method
 * to verify if the package is already installed
 */
void AptHandlerImpl::CreatePersistedId()
{
    _aptContent->Id = _aptContent->Name + "-" + _aptContent->Version;
}
