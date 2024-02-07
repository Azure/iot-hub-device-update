/**
 * @file swupdate_consent_handler.cpp
 * @brief Implementation of ContentHandler API for update content swupdate_consent.
 *
 * @copyright Copyright (c) conplement AG.
 * Licensed under either of Apache-2.0 or MIT license at your option.
 */
#include <fstream>

#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include "aduc/string_utils.hpp"
#include "aduc/swupdate_consent_handler.hpp"
#include "aduc/workflow_utils.h"
#include <azure_c_shared_utility/threadapi.h> // ThreadAPI_Sleep
#include <parson.h>
#include <signal.h> // raise()

#define ADUC_SWUPDATE_CONSENT_CONF_FILE "/etc/omnect/consent/consent_conf.json"
#define ADUC_SWUPDATE_CONSENT_USER_FILE "/etc/omnect/consent/swupdate/user_consent.json"
#define ADUC_SWUPDATE_CONSENT_REQUEST_FILE "/etc/omnect/consent/request_consent.json"
#define ADUC_SWUPDATE_CONSENT_HISTORY_FILE "/etc/omnect/consent/history_consent.json"

EXTERN_C_BEGIN

/////////////////////////////////////////////////////////////////////////////
// BEGIN Shared Library Export Functions
//
// These are the function symbols that the device update agent will
// lookup and call.
//

/**
 * @brief Instantiates an SWUpdateConsent Update Content Handler
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "swupdate_consent-handler");
    Log_Info("Instantiating a SWUpdateConsent Update Content Handler");
    try
    {
        return SWUpdateConsentHandlerImpl::CreateContentHandler();
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

    return nullptr;
}

/**
 * @brief Gets the extension contract info.
 *
 * @param[out] contractInfo The extension contract info.
 * @return ADUC_Result The result.
 */
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo)
{
    contractInfo->majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
    contractInfo->minorVer = ADUC_V1_CONTRACT_MINOR_VER;
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

//
// END Shared Library Export Functions
/////////////////////////////////////////////////////////////////////////////

EXTERN_C_END

/**
 * @brief Destructor for the SWUpdateConsent Handler Impl class.
 */
SWUpdateConsentHandlerImpl::~SWUpdateConsentHandlerImpl() // override
{
    ADUC_Logging_Uninit();
}

/**
 * @brief Creates a new SWUpdateConsentHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a SWUpdateConsentHandlerImpl directly.
 *
 * @return ContentHandler* SWUpdateConsentHandlerImpl object as a ContentHandler.
 */
ContentHandler* SWUpdateConsentHandlerImpl::CreateContentHandler()
{
    return new SWUpdateConsentHandlerImpl();
}

/**
 * @brief Convert char pointer to string
 *
 * @param s Pointer to char
 * @return std::string Returns the value from char pointer. Returns empty string if there was an error.
 */
std::string SWUpdateConsentHandlerImpl::ValueOrEmpty(const char* s)
{
    return s == nullptr ? std::string() : s;
}

/**
 * @brief Reads a first line of a file, trims trailing whitespace, and returns as string.
 *
 * @param filePath Path to the file to read value from.
 * @return std::string Returns the value from the file. Returns empty string if there was an error.
 */
std::string SWUpdateConsentHandlerImpl::ReadValueFromFile(const std::string& filePath)
{
    if (filePath.empty())
    {
        Log_Error("Empty file path.");
        return "";
    }

    if ((filePath.length()) + 1 > PATH_MAX)
    {
        Log_Error("Path is too long.");
        return "";
    }

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        Log_Error("File %s failed to open, error: %d", filePath.c_str(), errno);
        return "";
    }

    std::string result;
    std::getline(file, result);
    if (file.bad())
    {
        Log_Error("Unable to read from file %s, error: %d", filePath.c_str(), errno);
        return "";
    }

    // Trim whitespace
    ADUC::StringUtils::Trim(result);
    return result;
}

/**
 * @brief Load data from given path.
 *
 * @param filePath Path to the file to read value from.
 * @return JSON_Object A json object containing data.
 *         Caller must free the wrapping JSON_Value* object to free the memory.
 */
JSON_Object* SWUpdateConsentHandlerImpl::ReadJsonDataFile(const std::string& filePath)
{
    JSON_Value* rootValue = json_parse_file(filePath.c_str());

    if (nullptr == rootValue)
    {
        Log_Error("Cannot read file: %s", filePath);
    }
    return json_value_get_object(rootValue);
}

/**
 * @brief Check general consent from configuration consent file.
 *
 * @return "true" in case general consent is agreed, otherwise "false" will be returned
 */
bool SWUpdateConsentHandlerImpl::GetGeneralConsent(void)
{
    bool status = false;
    JSON_Object* data = ReadJsonDataFile(ADUC_SWUPDATE_CONSENT_CONF_FILE);

    if (nullptr == data)
    {
        Log_Error("No ADUC_SWUPDATE_CONSENT_CONF_FILE data file provided, or missing json content");
    }
    else
    {
        JSON_Array* array = json_object_get_array(data, "general_consent");
        if (nullptr == array)
        {
            Log_Error("general_consent not found in ADUC_SWUPDATE_CONSENT_CONF_FILE data file");
        }
        else
        {
            for (int i = 0; i < json_array_get_count(array); i++)
            {
                std::string generalConsentStatus = ValueOrEmpty(json_array_get_string(array, i));
                Log_Info("parsing json array genenral_consent: %s \n", generalConsentStatus.c_str());

                if (generalConsent == generalConsentStatus)
                {
                    Log_Info("genenral_consent found for swupdate\n");
                    status = true;
                    break;
                }
            }
        }
        json_value_free(json_object_get_wrapping_value(data));
    }

    return status;
}

/**
 * @brief Verifys that consent has been granted
 *
 * @param version Version to compare with the user consent version
 * @return "true" in case user consent is agreed, otherwise "false" will be returned
 *
 */
bool SWUpdateConsentHandlerImpl::UserConsentAgreed(const std::string& version)
{
    bool status = false;
    JSON_Object* data = ReadJsonDataFile(ADUC_SWUPDATE_CONSENT_USER_FILE);

    if (nullptr == data)
    {
        Log_Error("No ADUC_SWUPDATE_CONSENT_USER_FILE data file provided, or missing json content");
    }
    else
    {
        std::string userConsent = ValueOrEmpty(json_object_get_string(data, "consent"));
        if (userConsent == version)
        {
            status = true;
        }
        json_value_free(json_object_get_wrapping_value(data));
    }
    return status;
}

/**
 * @brief Clean user consent agreed
 *
 * @return ADUC_GeneralResult
 *
 */
ADUC_GeneralResult SWUpdateConsentHandlerImpl::CleanUserConsentAgreed(void)
{
    ADUC_GeneralResult result = ADUC_GeneralResult_Failure;
    JSON_Value* rootValue = json_parse_file(ADUC_SWUPDATE_CONSENT_USER_FILE);
    JSON_Object* data = json_value_get_object(rootValue);

    Log_Info("clean user consent agreed");

    if (nullptr == data)
    {
        Log_Error("No ADUC_SWUPDATE_CONSENT_USER_FILE data file provided, or missing json content");
    }
    else
    {
        // clean ADUC_SWUPDATE_CONSENT_USER_FILE only if consent available -> prevent file write operation
        if (json_object_has_value(data, "consent") != 0)
        {
            Log_Info("clean ADUC_SWUPDATE_CONSENT_USER_FILE");
            json_object_remove(data, "consent");
            json_serialize_to_file_pretty(rootValue, ADUC_SWUPDATE_CONSENT_USER_FILE);
        }
        result = ADUC_GeneralResult_Success;
        json_value_free(rootValue);
    }

    return result;
}

/**
 * @brief add records to an json array
 *
 * @param root  root of the parsed file
 * @param arrayName  the value name can be hierarchical as a dot separated string.
 * @param record  record to add
 * @return "true" in case successfully, otherwise "false" will be returned
 *
 */
bool SWUpdateConsentHandlerImpl::AppendArrayRecord(const JSON_Object* root, const char* arrayName, JSON_Value* record)
{
    bool result = false;
    JSON_Array* array;

    array = json_object_dotget_array(root, arrayName);

    if (json_array_append_value(array, record) == JSONSuccess)
    {
        result = true;
    }

    return result;
}

/**
 * @brief Request a user consent for given version
 *
 * @param version Version for request a user consent
 * @return ADUC_GeneralResult
 *
 */
ADUC_GeneralResult SWUpdateConsentHandlerImpl::UpdateConsentRequestJsonFile(const std::string& version)
{
    ADUC_GeneralResult result = ADUC_GeneralResult_Failure;
    JSON_Value* rootValue = json_parse_file(ADUC_SWUPDATE_CONSENT_REQUEST_FILE);
    JSON_Object* data = json_value_get_object(rootValue);

    if (nullptr == data)
    {
        Log_Error("No ADUC_SWUPDATE_CONSENT_REQUEST_FILE data file provided");
    }
    else
    {
        JSON_Array* array = json_object_get_array(data, "user_consent_request");
        if (nullptr == array)
        {
            Log_Error("No user_consent_request in ADUC_SWUPDATE_CONSENT_REQUEST_FILE data file provided");
        }
        else
        {
            JSON_Value* swupdateValue = json_value_init_object();
            JSON_Object* swupdateObject = json_value_get_object(swupdateValue);
            json_object_set_string(swupdateObject, "swupdate", version.c_str());
            if (true == AppendArrayRecord(data, "user_consent_request", swupdateValue))
            {
                json_serialize_to_file_pretty(rootValue, ADUC_SWUPDATE_CONSENT_REQUEST_FILE);
                result = ADUC_GeneralResult_Success;
            }
            else
            {
                Log_Error("append to user_consent_request in ADUC_SWUPDATE_CONSENT_REQUEST_FILE not working");
            }
        }
        json_value_free(rootValue);
    }

    return result;
}

/**
 * @brief Clean request user consent
 *
 * @return ADUC_GeneralResult
 *
 */
ADUC_GeneralResult SWUpdateConsentHandlerImpl::CleanConsentRequestJsonFile(void)
{
    ADUC_GeneralResult result = ADUC_GeneralResult_Failure;
    JSON_Value* rootValue = json_parse_file(ADUC_SWUPDATE_CONSENT_REQUEST_FILE);
    JSON_Object* data = json_value_get_object(rootValue);

    Log_Info("clean consent request");

    if (nullptr == data)
    {
        Log_Error("No ADUC_SWUPDATE_CONSENT_REQUEST_FILE data file provided");
    }
    else
    {
        JSON_Array* array = json_object_get_array(data, "user_consent_request");
        if (nullptr == array)
        {
            Log_Error("No user_consent_request in ADUC_SWUPDATE_CONSENT_REQUEST_FILE data file provided");
        }
        else
        {
            // clean ADUC_SWUPDATE_CONSENT_REQUEST_FILE only if array is not empty -> prevent file write operation
            if (json_array_get_count(array) != 0)
            {
                Log_Info("clean ADUC_SWUPDATE_CONSENT_REQUEST_FILE");
                json_array_clear(array);
                json_serialize_to_file_pretty(rootValue, ADUC_SWUPDATE_CONSENT_REQUEST_FILE);
            }
            result = ADUC_GeneralResult_Success;
        }
        json_value_free(rootValue);
    }

    return result;
}

/**
 * @brief store user consent version in history file
 *
 * @param version user consent version
 * @return ADUC_GeneralResult
 *
 */
ADUC_GeneralResult SWUpdateConsentHandlerImpl::UpdateConsentHistoryJsonFile(const std::string& version)
{
    ADUC_GeneralResult result = ADUC_GeneralResult_Failure;
    JSON_Value* rootValue = json_parse_file(ADUC_SWUPDATE_CONSENT_HISTORY_FILE);
    JSON_Object* data = json_value_get_object(rootValue);

    if (nullptr == data)
    {
        Log_Error("No ADUC_SWUPDATE_CONSENT_HISTORY_FILE data file provided");
    }
    else
    {
        JSON_Object* data2 = json_object_get_object(data, "user_consent_history");
        if (nullptr == data2)
        {
            Log_Error("No user_consent_agreed object in ADUC_SWUPDATE_CONSENT_HISTORY_FILE data file provided");
        }
        else
        {
            JSON_Array* array = json_object_get_array(data2, "swupdate");
            if (nullptr == array)
            {
                Log_Error("No swupdate array in ADUC_SWUPDATE_CONSENT_HISTORY_FILE data file provided");
            }
            else
            {
                json_array_append_string(array, version.c_str());
                json_serialize_to_file_pretty(rootValue, ADUC_SWUPDATE_CONSENT_HISTORY_FILE);
                result = ADUC_GeneralResult_Success;
            }
        }
        json_value_free(rootValue);
    }

    return result;
}

/**
 * @brief get version from installed criteria
 *
 * @param installedCriteria installed criteria info
 * @return std::string Returns the version from given installed criteria. Returns empty string if there was an error.
 *
 */
std::string SWUpdateConsentHandlerImpl::GetVersion(const std::string& installedCriteria)
{
    std::string version(strchr(installedCriteria.c_str(), ' '));

    if (version.empty())
    {
        Log_Error("couldn't read version from installedCriteria");
        return "";
    }
    else
    {
        Log_Info("read version \"%s\" from installedCriteria", version.c_str());
        // Trim whitespace
        ADUC::StringUtils::Trim(version);
        return version;
    }
}

/**
 * @brief Implementation of download action.
 * @return ADUC_Result return ADUC_Result_Download_Success.
 */
ADUC_Result SWUpdateConsentHandlerImpl::Download(const tagADUC_WorkflowData* workflowData)
{
    bool consent = false;
    ADUC_WorkflowHandle workflowHandle = workflowData->WorkflowHandle;
    char* installedCriteria_workflow = workflow_get_installed_criteria(workflowData->WorkflowHandle);
    std::string installedCriteria = ValueOrEmpty(installedCriteria_workflow);
    std::string version = GetVersion(installedCriteria);

    Log_Info("swupdate waiting for user consent");

    // cleanup request JSON file to prevent duplicate information in array due to e.g. device reboot
    CleanConsentRequestJsonFile();

    // log message already present in function, return value currently not needed to evaluate
    UpdateConsentRequestJsonFile(version);

    while (false == consent)
    {
        // check if general user consent agreed
        if (false == GetGeneralConsent())
        {
            if (workflow_is_cancel_requested(workflowHandle))
            {
                // see issue: https://github.com/Azure/iot-hub-device-update/issues/511
                // Workaround until we get a final fix from MS
                Log_Info("Restarting ADU Agent due to cancel request!");
                raise(SIGUSR1);
            }

            if (true == UserConsentAgreed(version))
            {
                // log message already present in function, return value currently not needed to evaluate
                UpdateConsentHistoryJsonFile(version);
                consent = true;
            }
        }
        else
            consent = true;

        // 1s timeout
        ThreadAPI_Sleep(1000);
    }

    CleanConsentRequestJsonFile();
    Log_Info("swupdate user consent succeeded");

    workflow_free_string(installedCriteria_workflow);
    return ADUC_Result{ ADUC_Result_Download_Success };
}

/**
 * @brief Implementation of install
 * @return ADUC_Result return ADUC_Result_Install_Success.
 */
ADUC_Result SWUpdateConsentHandlerImpl::Install(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate consent doesn't require a specific operation to install. (no-op) ");
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Install_Success };
}

/**
 * @brief Implementation of apply.
 * @return ADUC_Result return ADUC_Result_Apply_Success.
 */
ADUC_Result SWUpdateConsentHandlerImpl::Apply(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate consent doesn't require a specific operation to apply. (no-op) ");
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Apply_Success };
}

/**
 * @brief Implementation of cancel
 * @return ADUC_Result return ADUC_Result_Cancel_Success.
 */
ADUC_Result SWUpdateConsentHandlerImpl::Cancel(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate consent doesn't require a specific operation to cancel. (no-op) ");
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Cancel_Success };
}

/**
 * @brief Implementation of IsInstalled check.
 * @return ADUC_Result The result based on evaluating the installed criteria an GeneralConsent.
 */
ADUC_Result SWUpdateConsentHandlerImpl::IsInstalled(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { ADUC_Result_IsInstalled_NotInstalled };
    char* installedCriteria_workflow = workflow_get_installed_criteria(workflowData->WorkflowHandle);
    std::string installedCriteria = ValueOrEmpty(installedCriteria_workflow);
    std::string version = ReadValueFromFile(ADUC_VERSION_FILE);

    if (true == GetGeneralConsent())
    {
        Log_Info("consent for swupdate is available in configuration file");
        CleanUserConsentAgreed();
        CleanConsentRequestJsonFile();
        result = { ADUC_Result_IsInstalled_Installed };
    }
    else if (version == installedCriteria)
    {
        Log_Info("swupdate consent for Installed criteria %s was installed", installedCriteria.c_str());
        CleanConsentRequestJsonFile();
        result = { ADUC_Result_IsInstalled_Installed };
    }
    else
    {
        Log_Info(
            "swupdate consent Installed criteria %s was not installed, the current criteria is %s",
            installedCriteria.c_str(),
            version.c_str());
    }
    workflow_free_string(installedCriteria_workflow);
    return result;
}

/**
 * @brief Backup implementation.
 * Calls into the swupdate wrapper script to perform backup.
 * For swupdate, no operation is required.
 *
 * @return ADUC_Result The result of the backup.
 * It will always return ADUC_Result_Backup_Success.
 */
ADUC_Result SWUpdateConsentHandlerImpl::Backup(const tagADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    ADUC_Result result = { ADUC_Result_Backup_Success };
    Log_Info("SWUpdate consent doesn't require a specific operation to backup. (no-op) ");
    return result;
}

/**
 * @brief Restore implementation.
 * Calls into the swupdate wrapper script to perform restore.
 * Will flip bootloader flag to boot into the previous partition for A/B update.
 *
 * @return ADUC_Result The result of the restore.
 */
ADUC_Result SWUpdateConsentHandlerImpl::Restore(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate consent doesn't require a specific operation to restore. (no-op) ");
    UNREFERENCED_PARAMETER(workflowData);
    ADUC_Result result = { ADUC_Result_Restore_Success };
    return result;
}
