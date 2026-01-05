/**
 * @file swupdate_handler_v3.hpp
 * @brief Defines SWUpdateHandlerV3Impl.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_SWUPDATE_HANDLER_V3_HPP
#define ADUC_SWUPDATE_HANDLER_V3_HPP

#include "aduc/content_handler.hpp"
#include <aduc/result.h>
#include <string>
#include <unordered_map>
#include <vector>

typedef void* ADUC_WorkflowHandle;

/**
 * @class SWUpdateHandlerV3Impl
 * @brief The swupdate v3 specific implementation of ContentHandler interface.
 * 
 * V3 improvements:
 * - Uses standard U-Boot environment variables (not rpipart-specific)
 * - Implements boot health checking with automatic rollback
 * - Handler-specific error codes
 * - Minimal dependencies (will use SDK in future)
 */
class SWUpdateHandlerV3Impl : public ContentHandler
{
public:
    /**
     * @brief Default constructor for CreateContentHandler
     */
    static ContentHandler* CreateContentHandler();

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    SWUpdateHandlerV3Impl(const SWUpdateHandlerV3Impl&) = delete;
    SWUpdateHandlerV3Impl& operator=(const SWUpdateHandlerV3Impl&) = delete;
    SWUpdateHandlerV3Impl(SWUpdateHandlerV3Impl&&) = delete;
    SWUpdateHandlerV3Impl& operator=(SWUpdateHandlerV3Impl&&) = delete;

    ~SWUpdateHandlerV3Impl() override;

    ADUC_Result Download(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Backup(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Restore(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const tagADUC_WorkflowData* workflowData) override;

    /**
     * @brief Reads a value from the file and returns it in string format
     * @param filePath the file to read from
     * @returns a string representation of the value
     */
    static std::string ReadValueFromFile(const std::string& filePath);

    /**
     * @brief Reads the configuration from the @p configFile into @p values
     * @param configFile the path to the config file
     * @param values the map to read the data into
     * @returns a value of ADUC_Result
     */
    static ADUC_Result ReadConfig(const std::string& configFile, std::unordered_map<std::string, std::string>& values);

    /**
     * @brief Prepares the command arguments
     * @param workflowHandle the workflow handle having commands prepped
     * @param resultFilePath the result file path to use for the command
     * @param workFolder the folder to do the work in
     * @param commandFilePath the path to the command to execute
     * @param args the arguments for the command
     * @returns a value of ADUC_Result
     */
    static ADUC_Result PrepareCommandArguments(
        const ADUC_WorkflowHandle workflowHandle,
        std::string resultFilePath,
        std::string workFolder,
        std::string& commandFilePath,
        std::vector<std::string>& args);

    /**
     * @brief Checks if we're in post-reboot state and performs health check
     * @returns ADUC_Result indicating health check status
     */
    static ADUC_Result CheckPostRebootState();

    /**
     * @brief Gets U-Boot environment variable value
     * @param varName Name of the U-Boot variable
     * @returns String value or empty string on error
     */
    static std::string GetUBootEnv(const std::string& varName);

    /**
     * @brief Sets U-Boot environment variable
     * @param varName Name of the U-Boot variable
     * @param value Value to set
     * @returns true on success, false on failure
     */
    static bool SetUBootEnv(const std::string& varName, const std::string& value);

    /**
     * @brief Saves U-Boot environment to persistent storage
     * @returns true on success, false on failure
     */
    static bool SaveUBootEnv();

private:
    /**
     * @brief Constructor is private, must use CreateContentHandler factory method.
     */
    SWUpdateHandlerV3Impl() = default;

    /**
     * @brief Cancel the Apply action for @p workflowData
     * @param workflowData workflowData to cancel
     * @returns ADUC_Result
     */
    ADUC_Result CancelApply(const tagADUC_WorkflowData* workflowData);
};

#endif // ADUC_SWUPDATE_HANDLER_V3_HPP
