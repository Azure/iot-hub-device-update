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
#include "aduc/content_handler_factory.hpp"
#include <aduc/result.h>
#include <memory>
#include <string>

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

    ADUC_Result Download(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override;

private:
    // Private constructor, must call CreateContentHandler factory method.
    StepsHandlerImpl()
    {
    }
};

#endif // ADUC_STEPS_HANDLER_HPP
