/**
 * @file swupdate_handler.hpp
 * @brief Defines SWUpdateHandlerImpl.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_SWUPDATE_HANDLER_HPP
#define ADUC_SWUPDATE_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include <aduc/result.h>
#include <string>
#include <unordered_map>
#include <vector>

typedef void* ADUC_WorkflowHandle;

/**
 * @class SWUpdateHandlerImpl
 * @brief The swupdate specific implementation of ContentHandler interface.
 */
class SWUpdateHandlerImpl : public ContentHandler
{
public:
    /**
     * @brief Default constructor for CreateContentHandler
     */
    static ContentHandler* CreateContentHandler();

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    SWUpdateHandlerImpl(const SWUpdateHandlerImpl&) = delete;
    SWUpdateHandlerImpl& operator=(const SWUpdateHandlerImpl&) = delete;
    SWUpdateHandlerImpl(SWUpdateHandlerImpl&&) = delete;
    SWUpdateHandlerImpl& operator=(SWUpdateHandlerImpl&&) = delete;

    ~SWUpdateHandlerImpl() override;

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

private:
    /**
     * @brief Cancel the Apply action for @p workflowData
     * @param workflowData workflow data to cancel the apply for
     * @returns a value of ADUC_Result
     */
    ADUC_Result CancelApply(const tagADUC_WorkflowData* workflowData);

protected:
    //!< Protected constructor, must call CreateContentHandler factory method or from derived simulator class
    SWUpdateHandlerImpl()
    {
    }

    /**
     * @brief Performs @p action for workflow @p workflowData
     * @param action action to perform
     * @param workflowData workflow data to perform the action for
     * @returns a value of ADUC_Result
     */
    static ADUC_Result PerformAction(const std::string& action, const tagADUC_WorkflowData* workflowData);
};

#endif // ADUC_SWUPDATE_HANDLER_HPP
