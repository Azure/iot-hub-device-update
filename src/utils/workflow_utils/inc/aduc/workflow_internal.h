/**
 * @file workflow_internal.h
 * @brief internal workflow structures and function signatures for use in tests.
 *
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */

#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <aduc/types/workflow.h>
#include <azure_c_shared_utility/strings.h>
#include <parson.h>

/**
 * @brief A struct containing data needed for an update workflow.
 *
 */
typedef struct tagADUC_Workflow
{
    //
    // Parsed json state from the most recent, applicable ProcessDeployment request.
    // Immutable until replaced by a replacement workflow deployment.
    //
    JSON_Object* UpdateActionObject; /**< The update action JSON object. */
    JSON_Object* UpdateManifestObject; /**< The update manifest JSON object. */
    JSON_Object* PropertiesObject; /**< The Property JSON object. */
    JSON_Object* ResultsObject; /**< The results JSON object. */

    //
    // Mutable state used by the agent workflow orchestration.
    //
    ADUCITF_State State; /**< The current state machine state of the workflow. */
    ADUCITF_WorkflowStep CurrentWorkflowStep; /**< The current state machine workflow step. */
    ADUC_Result Result; /**< The result of the workflow. */
    STRING_HANDLE ResultDetails; /**< The result details of the workflow. */
    STRING_HANDLE InstalledUpdateId; /**< The installed updateId to report on workflow success. */

    //
    // Nested steps workflow state.
    //
    struct tagADUC_Workflow* Parent; /**< Pointer to the parent workflow update data. NULL if this is the root. */
    struct tagADUC_Workflow** Children; /**< Pointer to children workflow update data. NULL if this is a leaf. */
    size_t ChildrenMax; /**< The max children. */
    size_t ChildCount; /**< The count of children. */
    int Level; /**< The level of the workflow in the tree. */
    int StepIndex; /**< The step index for this workflow. */

    //
    // Operation worker state including state for handling cancellation and completion.
    //
    bool OperationInProgress; /**< Is an upper-level method currently in progress? */
    bool OperationCancelled; /**< Was the operation in progress requested to cancel? */
    ADUC_WorkflowCancellationType CancellationType; /**< What type of cancellation is it? */
    struct tagADUC_Workflow*
        DeferredReplacementWorkflow; /**< A replacement workflow that came in while another deployment was in progress. */
} ADUC_Workflow;
