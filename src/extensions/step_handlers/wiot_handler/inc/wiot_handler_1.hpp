/**
 * @file wiot_handler_1.hpp
 * @brief Defines WiotHandler1
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_IOT_HANDLER_HPP
#define ADUC_IOT_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include <aduc/result.h>

// Forward reference
typedef struct tagADUC_WorkflowData ADUC_WorkflowData;
using ADUC_WorkflowHandle = void*;

namespace noncopyable_ // protection from unintended ADL
{
class noncopyable
{
protected:
    constexpr noncopyable() = default;
    ~noncopyable() = default;
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};
} // namespace noncopyable_
typedef noncopyable_::noncopyable noncopyable;

/**
 * @class WiotHandler1
 * @brief The swupdate specific implementation of ContentHandler interface.
 */
class WiotHandler1 : public ContentHandler, noncopyable
{
public:
    static ContentHandler* CreateContentHandler()
    {
        return new WiotHandler1;
    }

    ~WiotHandler1() override;

    // ContentHandler methods

    ADUC_Result Download(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Backup(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Restore(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override;

    static bool IsValidUpdateTypeInfo(const ADUC_WorkflowHandle& workflowHandle);

protected:
    // Protected constructor, must call CreateContentHandler factory method or from derived simulator class
    WiotHandler1()
    {
    }

private:
};

#endif // ADUC_IOT_HANDLER_HPP
