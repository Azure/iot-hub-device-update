/**
 * @file script_handler.hpp
 * @brief Defines ScriptHandlerImpl.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_SCRIPT_HANDLER_HPP
#define ADUC_SCRIPT_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include <aduc/result.h>
#include <string>
#include <vector>

typedef void* ADUC_WorkflowHandle;

/**
 * @brief Contains the result of a call to ScriptHandler_PerformAction.
 *
 */
typedef struct ADUC_PerformAction_Results
{
    ADUC_Result result; // Result of the action
    std::vector<std::string> args; // Arguments to the action
    std::string scriptFilePath; // Path to the script file
    std::vector<std::string> commandLineArgs; // Command line arguments
    std::string
        scriptOutput; // When calling ScriptHandler_PerformAction, if prepareArgsOnly is false, this will contains the action output string.
} ADUC_PerformActionResult;

/**
 * @class ScriptHandlerImpl
 * @brief The Step Handler implementation for 'microsoft/script:1' update type.
 */
class ScriptHandlerImpl : public ContentHandler
{
public:
    static ContentHandler* CreateContentHandler();

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    ScriptHandlerImpl(const ScriptHandlerImpl&) = delete;
    ScriptHandlerImpl& operator=(const ScriptHandlerImpl&) = delete;
    ScriptHandlerImpl(ScriptHandlerImpl&&) = delete;
    ScriptHandlerImpl& operator=(ScriptHandlerImpl&&) = delete;

    ~ScriptHandlerImpl() override = default;

    ADUC_Result Download(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Backup(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Restore(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const tagADUC_WorkflowData* workflowData) override;

    static ADUC_Result PrepareScriptArguments(
        ADUC_WorkflowHandle workflowHandle,
        std::string resultFilePath,
        std::string workFolder,
        std::string& scriptFilePath,
        std::vector<std::string>& args);

private:
    ScriptHandlerImpl()
    {
    }

    static ADUC_Result PerformAction(const std::string& action, const tagADUC_WorkflowData* workflowData);
};

/**
 * @brief Perform the specified action.
 *
 * @param action Name of the action to perform
 * @param workflowData The workflow data
 * @param prepareArgsOnly if true, only prepare the arguments and return them in the result
 * @return ADUC_PerformAction_Results The result of the action and outputs of the script.
 */
ADUC_PerformAction_Results
ScriptHandler_PerformAction(const std::string& action, const tagADUC_WorkflowData* workflowData, bool prepareArgsOnly);

#endif // ADUC_SCRIPT_HANDLER_HPP
