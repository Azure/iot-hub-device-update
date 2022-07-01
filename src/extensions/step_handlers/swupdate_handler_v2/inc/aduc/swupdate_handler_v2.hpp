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

    static std::string ReadValueFromFile(const std::string& filePath);
    static ADUC_Result ReadConfig(const std::string& configFile, std::unordered_map<std::string, std::string>& values);

    static ADUC_Result PrepareCommandArguments(
        const ADUC_WorkflowHandle workflowHandle,
        std::string resultFilePath,
        std::string workFolder,
        std::string& commandFilePath,
        std::vector<std::string>& args);

protected:
    // Protected constructor, must call CreateContentHandler factory method or from derived simulator class
    SWUpdateHandlerImpl()
    {
    }

    static ADUC_Result PerformAction(const std::string& action, const tagADUC_WorkflowData* workflowData);
};

#endif // ADUC_SWUPDATE_HANDLER_HPP
