/**
 * @file content_handler.hpp
 * @brief Defines ContentHandler interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_CONTENT_HANDLER_HPP
#define ADUC_CONTENT_HANDLER_HPP

#include <string>

#include "aduc/adu_core_exports.h"
#include "aduc/result.h"
#include "aduc/types/workflow.h"

/**
 * @interface ContentHandler
 * @brief Interface for content specific handler implementations.
 */
class ContentHandler
{
public:
    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    ContentHandler(const ContentHandler&) = delete;
    ContentHandler& operator=(const ContentHandler&) = delete;
    ContentHandler(ContentHandler&&) = delete;
    ContentHandler& operator=(ContentHandler&&) = delete;

    virtual ADUC_Result Download(const ADUC_WorkflowData* workflowData) = 0;
    virtual ADUC_Result Install(const ADUC_WorkflowData* workflowData) = 0;
    virtual ADUC_Result Apply(const ADUC_WorkflowData* workflowData) = 0;
    virtual ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) = 0;
    virtual ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) = 0;

    virtual ~ContentHandler()
    {
    }

protected:
    ContentHandler() = default;
};

#endif // ADUC_CONTENT_HANDLER_HPP
