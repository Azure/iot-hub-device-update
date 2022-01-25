/**
 * @file steps_handler.hpp
 * @brief Defines StepsHandlerImpl.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_STEPS_HANDLER_HPP
#define ADUC_STEPS_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include <aduc/result.h>

/**
 * @class StepsHandlerImpl
 * @brief The Update Content Handler that performs 'Multi-Steps Ordered Execution'.
 */
class StepsHandlerImpl : public ContentHandler
{
public:
    static ContentHandler* CreateContentHandler();

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    StepsHandlerImpl(const StepsHandlerImpl&) = delete;
    StepsHandlerImpl& operator=(const StepsHandlerImpl&) = delete;
    StepsHandlerImpl(StepsHandlerImpl&&) = delete;
    StepsHandlerImpl& operator=(StepsHandlerImpl&&) = delete;

    ~StepsHandlerImpl() override;

    ADUC_Result Download(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const tagADUC_WorkflowData* workflowData) override;

private:
    // Private constructor, must call CreateContentHandler factory method.
    StepsHandlerImpl()
    {
    }
};

#endif // ADUC_STEPS_HANDLER_HPP
