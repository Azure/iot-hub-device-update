/**
 * @file auto_workflowhandle.hpp
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

/**
 * @brief Auto workflow handle struct encapsulates the workflow handle for unit tests
*/
struct AutoWorkflowHandle
{
    /**
     * @brief Construct a new AutoWorkflowHandle object
     * @param workflowHandle workflow handle to be managed
    */
    AutoWorkflowHandle(ADUC_WorkflowHandle workflowHandle) : handle(workflowHandle)
    {
    }

    /**
     * @brief Destructor for AutoWorkflowHandle object
    */
    ~AutoWorkflowHandle()
    {
        if (handle != nullptr)
        {
            workflow_free(handle);
            handle = nullptr;
        }
    }

private:
    ADUC_WorkflowHandle handle; // !< workflow handle
};

} // namespace aduc

#endif // AUTO_WORKFLOWHANDLE_HPP
