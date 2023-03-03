/**
 * @file audo_dir.hpp
 * @brief header for AutoWorkflowHandle. On scope exit, uninitializes the ADUC_WorkflowHandle.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef AUTO_WORKFLOWHANDLE_HPP
#define AUTO_WORKFLOWHANDLE_HPP

#include <aduc/workflow_utils.h> // workflow_uninit
#include <string>

namespace aduc
{
struct AutoWorkflowHandle
{
    AutoWorkflowHandle(ADUC_WorkflowHandle workflowHandle) : handle(workflowHandle)
    {
    }

    ~AutoWorkflowHandle()
    {
        if (handle != nullptr)
        {
            workflow_free(handle);
            handle = nullptr;
        }
    }

private:
    ADUC_WorkflowHandle handle;
};

} // namespace aduc

#endif // AUTO_WORKFLOWHANDLE_HPP
