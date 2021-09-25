/**
 * @file script_handler.hpp
 * @brief Defines ScriptHandlerImpl.
 *
 * @copyright Copyright (c) Microsoft Corp.
 */
#ifndef ADUC_SCRIPT_HANDLER_HPP
#define ADUC_SCRIPT_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include <aduc/result.h>
#include <string>
#include <vector>

typedef void* ADUC_WorkflowHandle;

/**
 * @class ScriptHandlerImpl
 * @brief The Update Content Handler implementation for 'microsoft/script:1' update type.
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

    ADUC_Result Download(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override;

    static ADUC_Result PrepareScriptArguments(
        const ADUC_WorkflowHandle workflowHandle,
        std::string resultFilePath,
        std::string workFolder,
        std::string& scriptFilePath,
        std::vector<std::string>& args);

private:
    ScriptHandlerImpl()
    {
    }

    static ADUC_Result PerformAction(const std::string& action, const ADUC_WorkflowData* workflowData);
};

#endif // ADUC_SCRIPT_HANDLER_HPP
