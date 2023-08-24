/**
 * @file wim_handler_1.hpp
 * @brief Defines WimHandler1
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef WIM_HANDLER_1_HPP
#define WIM_HANDLER_1_HPP

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
 * @class WimHandler1
 * @brief The swupdate specific implementation of ContentHandler interface.
 */
class WimHandler1 : public ContentHandler, noncopyable
{
public:
    /**
     * @brief Creates the content handler
     * @returns a value of ContentHandler*
     */
    static ContentHandler* CreateContentHandler()
    {
        return new WimHandler1;
    }
    /** @brief Default destructor */
    ~WimHandler1() override;

    /**
     * @brief Returns the ID of the handler
     * @returns "microsoft/wim:1"
     */
    static const char* ID()
    {
        return "microsoft/wim:1";
    }

    // ContentHandler methods

    /**
    * @brief Download function for the content handler
    * @param workflowData the workflow to be used for the operation
    * @returns a value of ADUC_Result
    */
    ADUC_Result Download(const ADUC_WorkflowData* workflowData) override;
    /**
    * @brief Backup function for the content handler
    * @param workflowData the workflow to be used for the operation
    * @returns a value of ADUC_Result
    */
    ADUC_Result Backup(const ADUC_WorkflowData* workflowData) override;
    /**
    * @brief Install function for the content handler
    * @param workflowData the workflow to be used for the operation
    * @returns a value of ADUC_Result
    */
    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override;
    /**
    * @brief Apply function for the content handler
    * @param workflowData the workflow to be used for the operation
    * @returns a value of ADUC_Result
    */
    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override;
    /**
    * @brief Cancel function for the content handler
    * @param workflowData the workflow to be used for the operation
    * @returns a value of ADUC_Result
    */
    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override;
    /**
    * @brief Restore function for the content handler
    * @param workflowData the workflow to be used for the operation
    * @returns a value of ADUC_Result
    */
    ADUC_Result Restore(const ADUC_WorkflowData* workflowData) override;
    /**
    * @brief Function checks if the update is installed
    * @param workflowData the workflow to be used for the operation
    * @returns a value of ADUC_Result
    */
    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override;

    /**
     * @brief Returns if the update type info is valid
     * @param workflowHandle the handle for the workflow struct to be validated
     * @returns true if valid ; false otherwise
     */
    static bool IsValidUpdateTypeInfo(const ADUC_WorkflowHandle& workflowHandle);

protected:
    //!< Protected constructor, must call CreateContentHandler factory method or from derived simulator class
    WimHandler1()
    {
    }

private:
};

#endif // WIM_HANDLER_1_HPP
