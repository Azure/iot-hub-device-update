/**
 * @file content_handler.hpp
 * @brief Defines ContentHandler interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_CONTENT_HANDLER_HPP
#define ADUC_CONTENT_HANDLER_HPP

#ifdef __cplusplus
#    define EXTERN_C_BEGIN \
        extern "C"         \
        {
#    define EXTERN_C_END }
#else
#    define EXTERN_C_BEGIN
#    define EXTERN_C_END
#endif

#include "aduc/result.h"

// Forward declation.
struct tagADUC_WorkflowData;

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

    virtual ADUC_Result Download(const tagADUC_WorkflowData* workflowData) = 0;
    virtual ADUC_Result Install(const tagADUC_WorkflowData* workflowData) = 0;
    virtual ADUC_Result Apply(const tagADUC_WorkflowData* workflowData) = 0;
    virtual ADUC_Result Cancel(const tagADUC_WorkflowData* workflowData) = 0;
    virtual ADUC_Result IsInstalled(const tagADUC_WorkflowData* workflowData) = 0;

    virtual ~ContentHandler()
    {
    }

protected:
    ContentHandler() = default;
};

#endif // ADUC_CONTENT_HANDLER_HPP
