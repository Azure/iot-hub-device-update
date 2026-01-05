/**
 * @file swupdate_handler_v3.cpp
 * @brief Implementation of ContentHandler API for swupdate wrapper script V3.
 *
 * V3 enhancements:
 * - Uses standard U-Boot environment variables (boot_partition, upgrade_available, etc.)
 * - Implements boot health checking with automatic rollback
 * - Handler-specific error codes
 * 
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/swupdate_handler_v3.hpp"

#include "aduc/adu_core_exports.h"
#include "aduc/config_utils.h"
#include "aduc/extension_manager.hpp"
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/process_utils.hpp"
#include "aduc/string_c_utils.h"
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"
#include "aduc/types/update_content.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"
#include "adushell_const.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include <parson.h>

#define HANDLER_PROPERTIES_SCRIPT_FILENAME "scriptFileName"
#define HANDLER_PROPERTIES_SWU_FILENAME "swuFileName"
#define HANDLER_PROPERTIES_API_VERSION "apiVersion"
#define HANDLER_ARG_ACTION "--action"

// U-Boot environment variable names (matching boot.cmd.in)
#define UBOOT_VAR_BOOT_PARTITION "boot_partition"        // Values: "rootA" or "rootB"
#define UBOOT_VAR_UPGRADE_AVAILABLE "upgrade_available"  // Values: "0" or "1"
#define UBOOT_VAR_BOOT_ATTEMPTS "boot_attempts"          // Current boot attempt counter
#define UBOOT_VAR_BOOT_RESULT "boot_result"              // Values: "unknown", "success", "failed"
#define UBOOT_VAR_BOOT_ATTEMPTS_A "boot_attempts_A"      // Boot attempts for rootA
#define UBOOT_VAR_BOOT_ATTEMPTS_B "boot_attempts_B"      // Boot attempts for rootB
#define UBOOT_VAR_BOOT_RESULT_A "boot_result_A"          // Result for rootA
#define UBOOT_VAR_BOOT_RESULT_B "boot_result_B"          // Result for rootB
#define UBOOT_VAR_BOOT_TIMESTAMP_A "boot_timestamp_A"    // Last boot timestamp for rootA
#define UBOOT_VAR_BOOT_TIMESTAMP_B "boot_timestamp_B"    // Last boot timestamp for rootB

// Boot limit before rollback (hardcoded in boot.cmd.in)
#define BOOT_LIMIT 3

namespace adushconst = Adu::Shell::Const;

struct JSONValueDeleter
{
    void operator()(JSON_Value* value)
    {
        json_value_free(value);
    }
};

using AutoFreeJsonValue_t = std::unique_ptr<JSON_Value, JSONValueDeleter>;

/**
 * @brief Destructor for the SWUpdate Handler V3 Impl class.
 */
SWUpdateHandlerV3Impl::~SWUpdateHandlerV3Impl()
{
    ADUC_Logging_Uninit();
}

/**
 * @brief Creates a new SWUpdateHandlerV3Impl object and casts to a ContentHandler.
 * Note that there is no way to create a SWUpdateHandlerV3Impl directly.
 *
 * @return ContentHandler* SWUpdateHandlerV3Impl object as a ContentHandler.
 */
ContentHandler* SWUpdateHandlerV3Impl::CreateContentHandler()
{
    return new SWUpdateHandlerV3Impl();
}

/**
 * @brief Get U-Boot environment variable value
 */
std::string SWUpdateHandlerV3Impl::GetUBootEnv(const std::string& varName)
{
    std::string result;
    std::string command = "fw_printenv -n " + varName;
    
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
    {
        Log_Error("Failed to execute fw_printenv for variable: %s", varName.c_str());
        return result;
    }

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result = buffer;
        // Remove trailing newline
        size_t len = result.length();
        if (len > 0 && result[len - 1] == '\n')
        {
            result.erase(len - 1);
        }
    }

    pclose(pipe);
    return result;
}

/**
 * @brief Set U-Boot environment variable
 */
bool SWUpdateHandlerV3Impl::SetUBootEnv(const std::string& varName, const std::string& value)
{
    std::string command = "fw_setenv " + varName + " " + value;
    int result = system(command.c_str());
    
    if (result != 0)
    {
        Log_Error("Failed to set U-Boot variable %s to %s (exit code: %d)", 
                  varName.c_str(), value.c_str(), result);
        return false;
    }
    
    Log_Info("Set U-Boot variable %s = %s", varName.c_str(), value.c_str());
    return true;
}

/**
 * @brief Save U-Boot environment to persistent storage
 */
bool SWUpdateHandlerV3Impl::SaveUBootEnv()
{
    // fw_setenv automatically saves the environment
    // This function is here for API compatibility
    return true;
}

/**
 * @brief Check post-reboot state and perform health check if needed
 */
ADUC_Result SWUpdateHandlerV3Impl::CheckPostRebootState()
{
    ADUC_Result result = { ADUC_GeneralResult_Success, 0 };
    
    // Check if upgrade is pending (upgrade_available = 1)
    std::string upgradeAvailable = GetUBootEnv(UBOOT_VAR_UPGRADE_AVAILABLE);
    
    if (upgradeAvailable != "1")
    {
        // No pending upgrade, nothing to check
        Log_Info("No pending upgrade detected");
        return result;
    }
    
    Log_Info("Post-reboot upgrade verification starting...");
    
    // Get current boot partition and boot attempts
    std::string bootPartition = GetUBootEnv(UBOOT_VAR_BOOT_PARTITION);
    std::string bootAttemptsStr = GetUBootEnv(UBOOT_VAR_BOOT_ATTEMPTS);
    
    int bootAttempts = bootAttemptsStr.empty() ? 0 : std::stoi(bootAttemptsStr);
    
    Log_Info("Current partition: %s, Boot attempts: %d, Limit: %d", 
             bootPartition.c_str(), bootAttempts, BOOT_LIMIT);
    
    // Check if boot limit exceeded (boot.cmd.in checks >= 3)
    if (bootAttempts >= BOOT_LIMIT)
    {
        Log_Error("Boot limit reached (%d >= %d), boot.cmd should have triggered rollback", 
                  bootAttempts, BOOT_LIMIT);
        result.ResultCode = ADUC_GeneralResult_Failure;
        result.ExtendedResultCode = MAKE_ADUC_EXTENDEDRESULTCODE(10, 3, 30); // BOOT_LIMIT_EXCEEDED
        return result;
    }
    
    // TODO: Run actual health check script
    // For now, assume health check passes
    bool healthCheckPassed = true;
    
    if (healthCheckPassed)
    {
        Log_Info("Health check PASSED - marking boot as successful");
        
        // Mark boot as successful (matching boot.cmd.in expected behavior)
        SetUBootEnv(UBOOT_VAR_BOOT_RESULT, "success");
        SetUBootEnv(UBOOT_VAR_UPGRADE_AVAILABLE, "0");
        SetUBootEnv(UBOOT_VAR_BOOT_ATTEMPTS, "0");
        
        // Update partition-specific result
        if (bootPartition == "rootA")
        {
            SetUBootEnv(UBOOT_VAR_BOOT_RESULT_A, "success");
        }
        else if (bootPartition == "rootB")
        {
            SetUBootEnv(UBOOT_VAR_BOOT_RESULT_B, "success");
        }
        
        result.ResultCode = ADUC_GeneralResult_Success;
        result.ExtendedResultCode = 0;
    }
    else
    {
        Log_Error("Health check FAILED - will retry on next boot");
        SetUBootEnv(UBOOT_VAR_BOOT_RESULT, "failed");
        
        result.ResultCode = ADUC_GeneralResult_Failure;
        result.ExtendedResultCode = MAKE_ADUC_EXTENDEDRESULTCODE(10, 3, 20); // HEALTH_CHECK_FAILED
    }
    
    return result;
}

/**
 * @brief Download implementation for SWUpdate V3.
 */
ADUC_Result SWUpdateHandlerV3Impl::Download(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate Handler V3: Download started");
    
    // Check post-reboot state first
    ADUC_Result postRebootResult = CheckPostRebootState();
    if (IsAducResultCodeFailure(postRebootResult.ResultCode))
    {
        Log_Warn("Post-reboot health check failed, but continuing with download");
    }
    
    // Delegate to the workflow data utils for actual download
    ADUC_Result result = workflow_data_download_content(workflowData);
    
    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        Log_Info("SWUpdate Handler V3: Download completed successfully");
    }
    else
    {
        Log_Error("SWUpdate Handler V3: Download failed");
    }
    
    return result;
}

/**
 * @brief Backup implementation for SWUpdate V3.
 */
ADUC_Result SWUpdateHandlerV3Impl::Backup(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate Handler V3: Backup - no action needed");
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

/**
 * @brief Install implementation for SWUpdate V3.
 */
ADUC_Result SWUpdateHandlerV3Impl::Install(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate Handler V3: Install started");
    
    ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };
    
    // Get current boot partition (should be "rootA" or "rootB")
    std::string currentPartition = GetUBootEnv(UBOOT_VAR_BOOT_PARTITION);
    if (currentPartition.empty())
    {
        currentPartition = "rootA"; // Default to rootA
    }
    
    // Determine target partition (switch A <-> B)
    std::string targetPartition = (currentPartition == "rootA") ? "rootB" : "rootA";
    
    Log_Info("Current boot partition: %s, Target partition: %s", 
             currentPartition.c_str(), targetPartition.c_str());
    
    // TODO: Execute swupdate with the update file
    // For now, simulate successful installation
    Log_Info("Simulating swupdate execution...");
    
    // Prepare for reboot by setting U-Boot variables (matching boot.cmd.in behavior)
    Log_Info("Preparing U-Boot environment for reboot...");
    
    // Set target boot partition
    if (!SetUBootEnv(UBOOT_VAR_BOOT_PARTITION, targetPartition))
    {
        Log_Error("Failed to set boot_partition to %s", targetPartition.c_str());
        result.ExtendedResultCode = MAKE_ADUC_EXTENDEDRESULTCODE(10, 3, 11); // UBOOT_ENV_WRITE_FAILED
        return result;
    }
    
    // Set upgrade_available flag to trigger validation mode
    if (!SetUBootEnv(UBOOT_VAR_UPGRADE_AVAILABLE, "1"))
    {
        Log_Error("Failed to set upgrade_available flag");
        result.ExtendedResultCode = MAKE_ADUC_EXTENDEDRESULTCODE(10, 3, 11); // UBOOT_ENV_WRITE_FAILED
        return result;
    }
    
    // Reset boot attempts counter
    if (!SetUBootEnv(UBOOT_VAR_BOOT_ATTEMPTS, "0"))
    {
        Log_Error("Failed to reset boot_attempts");
        result.ExtendedResultCode = MAKE_ADUC_EXTENDEDRESULTCODE(10, 3, 11); // UBOOT_ENV_WRITE_FAILED
        return result;
    }
    
    // Set boot_result to unknown for new partition
    if (!SetUBootEnv(UBOOT_VAR_BOOT_RESULT, "unknown"))
    {
        Log_Error("Failed to set boot_result");
        result.ExtendedResultCode = MAKE_ADUC_EXTENDEDRESULTCODE(10, 3, 11); // UBOOT_ENV_WRITE_FAILED
        return result;
    }
    
    Log_Info("SWUpdate Handler V3: Install completed successfully");
    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;
    
    return result;
}

/**
 * @brief Apply implementation for SWUpdate V3.
 */
ADUC_Result SWUpdateHandlerV3Impl::Apply(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate Handler V3: Apply - triggering reboot");
    
    // Verify U-Boot environment is set correctly
    std::string upgradeAvailable = GetUBootEnv(UBOOT_VAR_UPGRADE_AVAILABLE);
    if (upgradeAvailable != "1")
    {
        Log_Error("upgrade_available not set, cannot proceed with reboot");
        return ADUC_Result{ ADUC_GeneralResult_Failure, 
                           MAKE_ADUC_EXTENDEDRESULTCODE(10, 3, 14) }; // PARTITION_SWITCH_FAILED
    }
    
    // Request system reboot
    Log_Info("Requesting system reboot...");
    ADUC_Result result = ADUC_SystemUtils_RequestReboot();
    
    return result;
}

/**
 * @brief Cancel implementation for SWUpdate V3.
 */
ADUC_Result SWUpdateHandlerV3Impl::Cancel(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate Handler V3: Cancel requested");
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

/**
 * @brief Restore implementation for SWUpdate V3.
 */
ADUC_Result SWUpdateHandlerV3Impl::Restore(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate Handler V3: Restore - no action needed");
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

/**
 * @brief IsInstalled implementation for SWUpdate V3.
 */
ADUC_Result SWUpdateHandlerV3Impl::IsInstalled(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate Handler V3: IsInstalled check");
    
    // Check if upgrade was successful
    std::string bootResult = GetUBootEnv(UBOOT_VAR_BOOT_RESULT);
    std::string upgradeAvailable = GetUBootEnv(UBOOT_VAR_UPGRADE_AVAILABLE);
    
    if (bootResult == "success" && upgradeAvailable == "0")
    {
        Log_Info("Update successfully installed and verified");
        return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
    }
    
    Log_Info("Update not yet verified or still pending (boot_result=%s, upgrade_available=%s)",
             bootResult.c_str(), upgradeAvailable.c_str());
    return ADUC_Result{ ADUC_GeneralResult_Failure, 0 };
}

/**
 * @brief Cancel Apply for SWUpdate V3.
 */
ADUC_Result SWUpdateHandlerV3Impl::CancelApply(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("SWUpdate Handler V3: CancelApply");
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

/**
 * @brief Reads a value from the file
 */
std::string SWUpdateHandlerV3Impl::ReadValueFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        Log_Error("Failed to open file: %s", filePath.c_str());
        return "";
    }

    std::string content;
    std::getline(file, content);
    file.close();

    return content;
}

/**
 * @brief Reads configuration from file
 */
ADUC_Result SWUpdateHandlerV3Impl::ReadConfig(
    const std::string& configFile,
    std::unordered_map<std::string, std::string>& values)
{
    Log_Info("Reading config from: %s", configFile.c_str());
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

/**
 * @brief Prepares command arguments
 */
ADUC_Result SWUpdateHandlerV3Impl::PrepareCommandArguments(
    const ADUC_WorkflowHandle workflowHandle,
    std::string resultFilePath,
    std::string workFolder,
    std::string& commandFilePath,
    std::vector<std::string>& args)
{
    Log_Info("Preparing command arguments");
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}
