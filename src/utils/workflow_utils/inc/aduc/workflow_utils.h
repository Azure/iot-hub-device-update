/**
 * @file workflow_utils.h
 * @brief Util functions for ADUC_Workflow data.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_WORKFLOW_UTILS_H
#define ADUC_WORKFLOW_UTILS_H

#include "aduc/adu_types.h"
#include "aduc/result.h"
#include "aduc/types/update_content.h"
#include "aduc/types/workflow.h"
#include <azure_c_shared_utility/strings.h>

#include <stdbool.h>
#include <string.h> // strlen
#include <sys/types.h> // ino_t

EXTERN_C_BEGIN

// fwd declaration
typedef void* ADUC_WorkflowHandle;

//
// Initialization.
//

/**
 * @brief Instantiate and initialize workflow object with info from @p updateManifestJson
 *
 * @param updateManifestJson A json string containing an update manifest data.
 * @param validateManifest  A indicates whether to validate the manifest signature.
 *  Note that a referenced step manifest file doesn't contain a signature. We sign at the top level.
 * @param handle A workflow object handle with information about the workflow.
 * @return ADUC_Result
 */
ADUC_Result workflow_init(const char* updateManifestJson, bool validateManifest, ADUC_WorkflowHandle* handle);

/**
 * @brief Instantiate and initialize workflow object with info from specified file.
 *
 * @param updateManifestFile Full path to a file containing an update manifest data.
 * @param validateManifest A boolean indicates whether to validate the manifest signature.
 * @param handle A workflow object handle with information about the workflow.
 * @return ADUC_Result
 */
ADUC_Result
workflow_init_from_file(const char* updateManifestFile, bool validateManifest, ADUC_WorkflowHandle* handle);

/**
 * @brief Instantiate and initialize workflow object with info from the @p sourceHandle inline step.
 *
 * @param base A source workflow object.
 * @param stepIndex A step index.
 * @param handle A workflow object handle with information about the workflow.
 * @return ADUC_Result
 */
ADUC_Result workflow_create_from_inline_step(ADUC_WorkflowHandle base, int stepIndex, ADUC_WorkflowHandle* handle);

/**
 * @brief Transfer action data from @p sourceHandle to @p targetHandle.
 * The sourceHandle will no longer contains transferred action data.
 * Caller should not use sourceHandle for other workflow related purposes.
 *
 * @param targetHandle A target workflow object.
 * @param sourceHandle A source workflow object.
 * @return Returns true if succeeded, or false if failed.
 */
bool workflow_transfer_data(ADUC_WorkflowHandle targetHandle, ADUC_WorkflowHandle sourceHandle);

/**
 * @brief Deep copy string. Caller must call workflow_free_string() when done.
 *
 * @param s Input string.
 * @return A copy of input string if succeeded. Otherwise, return NULL.
 */
char* workflow_copy_string(const char* s);

/**
 * @brief Free text buffer returned by workflow_get_* APIs.
 *
 * @param string
 */
void workflow_free_string(char* string);

/**
 * @brief Free workflow content.
 *
 * @param handle A workflow object handle.
 */
void workflow_uninit(ADUC_WorkflowHandle handle);

/**
 * @brief Free workflow content and dealloc workflow object.
 *
 * @param handle A workflow data object handle.
 */
void workflow_free(ADUC_WorkflowHandle handle);

//
// Property setters and getters.
//

/**
 * @brief Get Update Action.
 *
 * @param handle A workflow data object handle.
 * @return ADUCITF_UpdateAction
 */
ADUCITF_UpdateAction workflow_get_action(ADUC_WorkflowHandle handle);

/**
 * @brief Get Update Type.
 *
 * @param handle A workflow data object handle.
 * @return const char* A current workflow update type.
 */
char* workflow_get_update_type(ADUC_WorkflowHandle handle);

/**
 * @brief Gets the update type of the specified workflow.
 *
 * @param handle A workflow object handle.
 * @return An UpdateType string. Caller does not own the string so must not free it.
 */
const char* workflow_peek_update_type(ADUC_WorkflowHandle handle);

/**
 * @brief gets the current workflow step.
 *
 * @param handle A workflow data object handle.
 * @return the workflow step.
 */
ADUCITF_WorkflowStep workflow_get_current_workflowstep(ADUC_WorkflowHandle handle);

/**
 * @brief sets the current workflow step.
 *
 * @param handle A workflow data object handle.
 * @param workflowStep The workflow step.
 */
void workflow_set_current_workflowstep(ADUC_WorkflowHandle handle, ADUCITF_WorkflowStep workflowStep);

/**
 * @brief Explicit set workflow id for this workflow.
 *
 * @param handle A workflow data object handle.
 * @param id
 * @return Return true if succeeded.
 */
bool workflow_set_id(ADUC_WorkflowHandle handle, const char* id);

/**
 * @brief Get the workflow id.
 *
 * @param handle A workflow data object handle.
 * @return char* contains workflow id. Caller must call workflow_free_string() to free the memory once done.
 */
char* workflow_get_id(ADUC_WorkflowHandle handle);

/**
 * @brief Get a read-only workflow id.
 *
 * @param handle A workflow data object handle.
 * @return Return the workflow id.
 */
const char* workflow_peek_id(ADUC_WorkflowHandle handle);

/**
 * @brief Explicit set workflow retryTimestamp for this workflow.
 *
 * @param handle A workflow data object handle.
 * @param retryTimestamp The retryTimestamp
 * @return Return true if succeeded.
 */
bool workflow_set_retryTimestamp(ADUC_WorkflowHandle handle, const char* retryTimestamp);

/**
 * @brief Get a read-only workflow retryTimestamp.
 *
 * @param handle A workflow data object handle.
 * @return Return the workflow retryTimestamp. Returns null if retryTimestamp did not exist in service request.
 */
const char* workflow_peek_retryTimestamp(ADUC_WorkflowHandle handle);

/**
 * @brief Set a work folder (a.k.a, sandbox) for this workflow.
 *
 * @param handle A workflow data object handle.
 * @param format A format string for work folder path.
 * @param ... An argument list.
 * @return Return true is succeeded.
 */
bool workflow_set_workfolder(ADUC_WorkflowHandle handle, const char* format, ...);

/**
 * @brief Get the work folder for this workflow.
 *
 * @param handle A workflow data object handle.
 * @return char* contains full path to work folder. Caller must call workflow_free_string() to free the memory once done.
 */
char* workflow_get_workfolder(const ADUC_WorkflowHandle handle);

/**
 * @brief Sets selected-components (in a form of serialized json string) to be used in this workflow.
 *
 * @param handle A workflow data object handle.
 * @param selectedComponents Json string contains one or more components.
 * @return Returns true if succeeded.
 */
bool workflow_set_selected_components(ADUC_WorkflowHandle handle, const char* selectedComponents);

/**
 * @brief Gets a reference to the selected-components JSON string.
 *
 * @param handle A workflow data object handle.
 * @return const char* Contain selected-components JSON. Caller must not free this string.
 */
const char* workflow_peek_selected_components(ADUC_WorkflowHandle handle);

/**
 * @brief Gets the update files count.
 *
 * @param handle A workflow data object handle.
 * @return size_t Total files count.
 */
size_t workflow_get_update_files_count(ADUC_WorkflowHandle handle);

/**
 * @brief Gets the update file entity at the specified index.
 *
 * @param handle A workflow data object handle.
 * @param index An index of the file to get.
 * @param entity An output file entity object. Caller must free the object with workflow_free_file_entity().
 * @return true If succeeded.
 */
bool workflow_get_update_file(ADUC_WorkflowHandle handle, size_t index, ADUC_FileEntity** entity);

/**
 * @brief Gets the update file entity by name.
 *
 * @param handle A workflow data object handle.
 * @param fileName File name.
* @param entity An output file entity object. Caller must free the object with workflow_free_file_entity().
 * @return true If succeeded.
 */
bool workflow_get_update_file_by_name(ADUC_WorkflowHandle handle, const char* fileName, ADUC_FileEntity** entity);

/**
 * @brief Gets the inode associated with the update file entity at the specified index.
 *
 * @param handle A workflow data object handle.
 * @param index An index of the file to get.
 * @return ino_t The inode, or ADUC_INODE_SENTINEL_VALUE if inode has not been set yet.
 */
ino_t workflow_get_update_file_inode(ADUC_WorkflowHandle handle, size_t index);

/**
 * @brief Sets the inode associated with the update file entity at the specified index.
 *
 * @param handle A workflow data object handle.
 * @param index An index of the file to get.
 * @param inode The inode.
 * @return bool true on success.
 */
bool workflow_set_update_file_inode(ADUC_WorkflowHandle handle, size_t index, ino_t inode);

/**
 * @brief Gets a bundle updates count.
 *
 * @param handle A workflow data object handle.
 * @return size_t Total bundle updates count.
 */
size_t workflow_get_bundle_updates_count(ADUC_WorkflowHandle handle);

/**
 * @brief Gets a bundle update file at specified index.
 *
 * @param handle A workflow data object handle.
 * @param index An index of the file to get.
 * @param entity An output file entity object. Caller must free the object with workflow_free_file_entity().
 * @return true If succeeded.
 */
bool workflow_get_bundle_updates_file(ADUC_WorkflowHandle handle, size_t index, ADUC_FileEntity** entity);

/**
 * @brief Free specified file entity object.
 *
 * @param entity A pointer to file entity object to be freed.
 */
void workflow_free_file_entity(ADUC_FileEntity* entity);

/**
 * @brief Get an Update Manifest property (string) without copying the value.
 * Caller must not free the pointer.
 *
 * @param handle A workflow data object handle.
 * @param[in] propertyName Name of the property to get.
 * @return const char* A reference to property value. Caller must not free this pointer.
 */
const char* workflow_peek_update_manifest_string(ADUC_WorkflowHandle handle, const char* propertyName);

/**
 * @brief Get a property of type 'string' in the workflow update manifest.
 *
 * @param handle A workflow object handle.
 * @param propertyName The name of a property to get.
 *
 * @return A copy of specified property. Caller must call workflow_free_string when done with the value.
 */
char* workflow_get_update_manifest_string_property(ADUC_WorkflowHandle handle, const char* propertyName);

/**
 * @brief Get a 'Compatibility' entry of the workflow at a specified @p index.
 *
 * @param handle A workflow object handle.
 * @param index Index of the compatibility set to.
 *
 * @return A copy of compatibility entry. Caller must call workflow_free_string when done with the value.
 */
char* workflow_get_update_manifest_compatibility(ADUC_WorkflowHandle handle, size_t index);

/**
 * @brief Get update manifest version.
 *
 * @param handle A workflow object handle.
 *
 * @return int The manifest version number. Return -1, if failed.
 */
int workflow_get_update_manifest_version(ADUC_WorkflowHandle handle);

/**
 * @brief Return an update id of this workflow.
 * This id should be reported to the cloud once the update installed successfully.
 *
 * @param handle A workflow object handle.
 * @param[out] updateId A pointer to the output ADUC_UpdateId struct.
 *                      Must call 'workflow_free_update_id' function to free the memory when done.
 *
 * @return ADUC_Result Return ADUC_GeneralResult_Success if success. Otherwise, return ADUC_GeneralResult_Failure with extendedResultCode.
 */
ADUC_Result workflow_get_expected_update_id(ADUC_WorkflowHandle handle, ADUC_UpdateId** updateId);

/**
 * @brief Return an update id of this workflow.
 * This id should be reported to the cloud once the update installed successfully.
 *
 * @param handle A workflow object handle.
 *
 * @return char* Expected update id string.
 *         Caller must call 'workflow_free_string' function to free the memory when done.
 */
char* workflow_get_expected_update_id_string(ADUC_WorkflowHandle handle);

/**
 * @brief Get installed-criteria string from this workflow.
 * @param handle A workflow object handle.
 * @return Returns installed-criteria string.
 *         Caller must call 'workflow_free_string' function to free the memory when done.
 */
char* workflow_get_installed_criteria(ADUC_WorkflowHandle handle);

/**
 * @brief Get the Update Manifest 'compatibility' array, in serialized json string format.
 *
 * @param handle A workflow handle.
 * @return char* If success, returns a serialized json string. Otherwise, returns NULL.
 *         Caller must call 'workflow_free_string' function to free the memory when done.
 */
char* workflow_get_compatibility(ADUC_WorkflowHandle handle);

/**
 * @brief Free memory allocated for @p updateId.
 *
 * @param updateId Update Id object to free.
 */
void workflow_free_update_id(ADUC_UpdateId* updateId);

void workflow_set_operation_in_progress(ADUC_WorkflowHandle handle, bool inProgress);

bool workflow_get_operation_in_progress(ADUC_WorkflowHandle handle);

void workflow_set_operation_cancel_requested(ADUC_WorkflowHandle handle, bool cancel);

bool workflow_get_operation_cancel_requested(ADUC_WorkflowHandle handle);

void workflow_clear_inprogress_and_cancelrequested(ADUC_WorkflowHandle handle);

//
// Tree
//

/**
 * @brief Get root workflow object handle.
 *
 * @param handle A workflow data object handle.
 * @return ADUC_WorkflowHandle A root workflow object handle. If @p handle doesn't have parent, return NULL.
 */
ADUC_WorkflowHandle workflow_get_root(ADUC_WorkflowHandle handle);

/**
 * @brief Get parent workflow object handle of @p handle.
 *
 * @param handle A workflow data object handle.
 * @return ADUC_WorkflowHandle A parent workflow object handle.
 */
ADUC_WorkflowHandle workflow_get_parent(ADUC_WorkflowHandle handle);

/**
 * @brief Get child workflow count. For example, for Bundle Update, this is a count of
 * Leaf (Components) Updates. For Leaf (Components) Update, this is a count of 'InstallItems'.
 *
 * @param handle A workflow data object handle.
 * @return A total count of child workflows.
 */
int workflow_get_children_count(ADUC_WorkflowHandle handle);

/**
 * @brief Get child workflow at specified @p index.
 * Note: To get the last child, pass (-1) index.
 *
 * @param handle A workflow data object handle.
 * @param index Index of child to get.
 * @return ADUC_WorkflowHandle A child workflow object handle.
 */
ADUC_WorkflowHandle workflow_get_child(ADUC_WorkflowHandle handle, int index);

/**
 * @brief Insert @p childHandle into @p handle children list.
 *
 * The @p childHandle will be freed when @p handle is free using workflow_free() function.
 *
 * The @p childHandler's level will be set to ( @p handle's level + 1 ).
 *
 * @param handle A parent workflow object handle.
 * @param index An index indicate the location the @p childHandle will be inserted at.
 *              To insert at the end of the list, pass '-1'.
 * @param childHandle A child workflow object handle.
 * @return true If succeeded.
 */
bool workflow_insert_child(ADUC_WorkflowHandle handle, int index, ADUC_WorkflowHandle childHandle);

// If success, returns removed file.
// Note: to remove the last child, pass (-1) index.
ADUC_WorkflowHandle workflow_remove_child(ADUC_WorkflowHandle handle, int index);

//
// State
//
bool workflow_set_state(ADUC_WorkflowHandle handle, ADUCITF_State state);

ADUCITF_State workflow_get_root_state(ADUC_WorkflowHandle handle);
ADUCITF_State workflow_get_state(ADUC_WorkflowHandle handle);

ADUC_Result workflow_get_result(ADUC_WorkflowHandle handle);
void workflow_set_result(ADUC_WorkflowHandle handle, ADUC_Result result);
ADUC_Result_t workflow_get_success_erc(ADUC_WorkflowHandle handle);
void workflow_set_success_erc(ADUC_WorkflowHandle handle, ADUC_Result_t erc);

/**
 * @brief Set workflow resultDetails string.
 *
 * @param handle A workflow object handle.
 * @param format A string format.
 * @param ... Arguments.
 */
void workflow_set_result_details(ADUC_WorkflowHandle handle, const char* format, ...);

/**
 * @brief Get a reference of the workflow result details string.
 *
 * @param handle A workflow data object handle.
 * @return const char* A reference to result details string. Caller must not free this pointer.
 */
const char* workflow_peek_result_details(ADUC_WorkflowHandle handle);

void workflow_set_installed_update_id(ADUC_WorkflowHandle handle, const char* installedUpdateId);

const char* workflow_peek_installed_update_id(ADUC_WorkflowHandle handle);

bool workflow_read_state_from_file(ADUC_WorkflowHandle handle, const char* stateFilename);

/**
 * @brief Request a workflow cancellation.
 *
 * @param handle A workflow data object handle.
 * @return A boolean indicates that the 'WORKFLOW_PROPERTY_FIELD_CANCEL_REQUESTED' property successfully set to 'true'.
 */
bool workflow_request_cancel(ADUC_WorkflowHandle handle);

/**
 * @brief Check whether the workflow cancellation is requested.
 *
 * @param handle A workflow data object handle.
 * @return A boolean indicates whether the workflow cancellation is requested.
 */
bool workflow_is_cancel_requested(ADUC_WorkflowHandle handle);

/**
 * @brief Request the agent to restart after the top level workflow is finished.
 *
 * @param handle A workflow data object handle.
 * @return 'True' if request success.
 */
bool workflow_request_agent_restart(ADUC_WorkflowHandle handle);

/**
 * @brief Request the agent to restart immediately after current operation (e.g., download, install, or apply) is finished.
 *
 * @param handle A workflow data object handle.
 * @return 'True' if request success.
 */
bool workflow_request_immediate_agent_restart(ADUC_WorkflowHandle handle);

/**
 * @brief Check whether the agent restart is requested.
 *
 * @param handle A workflow data object handle.
 * @return A boolean indicates whether the agent restart is requested.
 */
bool workflow_is_agent_restart_requested(ADUC_WorkflowHandle handle);

/**
 * @brief Check whether the agent immediate restart is requested.
 *
 * @param handle A workflow data object handle.
 * @return A boolean indicates whether the agent immediate restart is requested.
 */
bool workflow_is_immediate_agent_restart_requested(ADUC_WorkflowHandle handle);

/**
 * @brief Request the agent to reboot the device after the top level workflow is finished.
 *
 * @param handle A workflow data object handle.
 * @return 'True' if request success.
 */
bool workflow_request_reboot(ADUC_WorkflowHandle handle);

/**
 * @brief Request the agent to reboot the device immediately  after current operation (e.g., download, install, or apply) is finished.
 *
 * @param handle A workflow data object handle.
 * @return 'True' if request success.
 */
bool workflow_request_immediate_reboot(ADUC_WorkflowHandle handle);

/**
 * @brief Check whether the device reboot is requested.
 *
 * @param handle A workflow data object handle.
 * @return A boolean indicates whether the device reboot is requested.
 */
bool workflow_is_reboot_requested(ADUC_WorkflowHandle handle);

/**
 * @brief Check whether the immediate device reboot is requested.
 *
 * @param handle A workflow data object handle.
 * @return A boolean indicates whether the immediate device reboot is requested.
 */
bool workflow_is_immediate_reboot_requested(ADUC_WorkflowHandle handle);

/**
 * @brief Set workflow cancellation type.
 *
 * @param handle A workflow data object handle.
 * @param cancellationType A cancellation type.
 */
void workflow_set_cancellation_type(ADUC_WorkflowHandle handle, ADUC_WorkflowCancellationType cancellationType);

/**
 * @brief Set workflow cancellation type.
 *
 * @param handle A workflow data object handle.
 * @param cancellationType A cancellation type.
 */
ADUC_WorkflowCancellationType workflow_get_cancellation_type(ADUC_WorkflowHandle handle);

bool workflow_update_retry_deployment(ADUC_WorkflowHandle handle, const char* retryToken);

bool workflow_update_replacement_deployment(
    ADUC_WorkflowHandle currentWorkflowHandle, ADUC_WorkflowHandle nextWorkflowHandle);
void workflow_update_for_replacement(ADUC_WorkflowHandle handle);
void workflow_update_for_retry(ADUC_WorkflowHandle handle);

//
// Misc.
//

/**
 * @brief Compare id of @p handle0 and @p handle1
 *
 * @param handle0 The workflow handle containing the first workflow id.
 * @param handle1 The workflow handle containing the second workflow id.
 * @return 0 if ids are equal.
 */
int workflow_id_compare(ADUC_WorkflowHandle handle0, ADUC_WorkflowHandle handle1);

/**
 * @brief Compare id of @p handle and @p workflowId. No memory is allocated or freed by this function.
 *
 * @param handle The handle for first workflow id.
 * @param workflowId The c-string for the second workflow id.
 * @return bool Returns true if ids are equal.
 */
bool workflow_isequal_id(ADUC_WorkflowHandle handle, const char* workflowId);

/**
 * @brief Create a new workflow data handler using base workflow and serialized 'instruction' json string.
 *
 * @param base The base workflow containing valid Update Action and Manifest.
 * @param instruction A serialized json string containing single 'installItem' from an instruction file.
 * @param handle An output workflow object handle.
 * @return ADUC_Result
 */
ADUC_Result
workflow_create_from_instruction(ADUC_WorkflowHandle base, const char* instruction, ADUC_WorkflowHandle* handle);

/**
 * @brief Create a new workflow data handler using base workflow and serialized 'instruction' json string.
 *
 * @param base The base workflow containing valid Update Action and Manifest.
 * @param instruction A JSON_Value object containing single 'installItem' from an instruction file.
 * @param handle An output workflow object handle.
 * @return ADUC_Result
 */
ADUC_Result
workflow_create_from_instruction_value(ADUC_WorkflowHandle base, JSON_Value* instruction, ADUC_WorkflowHandle* handle);

//
// Update Manifest Version 4
//

/**
 * @brief Set workflow level.
 *
 * @param handle A workflow object handle.
 * @param level A workflow level
 */
void workflow_set_level(ADUC_WorkflowHandle handle, int level);

/**
 * @brief Set workflow step index.
 *
 * @param handle A workflow object handle.
 * @param stepIndex A workflow step index.
 */
void workflow_set_step_index(ADUC_WorkflowHandle handle, int stepIndex);

/**
 * @brief Get workflow level.
 *
 * @param handle A workflow object handle.
 * @return Returns -1 if the specified handle is invalid. Otherwise, return the workflow level.
 */
int workflow_get_level(ADUC_WorkflowHandle handle);

/**
 * @brief Get workflow step index.
 *
 * @param handle A workflow object handle.
 * @return Returns -1 if the specified handle is invalid. Otherwise, return the workflow step index.
 */
int workflow_get_step_index(ADUC_WorkflowHandle handle);

/**
 * @brief Get update manifest instructions steps count.
 *
 * @param handle A workflow object handle.
 *
 * @return Total count of the update instruction steps.
 */
size_t workflow_get_instructions_steps_count(ADUC_WorkflowHandle handle);

/**
 * @brief Get a read-only update manifest step type
 *
 * @param handle A workflow object handle.
 * @param stepIndex A step index.
 *
 * @return A step type. This can be 'inline' or 'reference'.
 */
const char* workflow_peek_step_type(ADUC_WorkflowHandle handle, size_t stepIndex);

/**
 * @brief Returns whether the specified step is an 'inline' step.
 *
 * @param handle A workflow object handle.
 * @param stepIndex A step index.
 *
 * @return bool Return true if the specified step exists, and is inline step.
 * Otherwise, return false.
 *
 */
bool workflow_is_inline_step(ADUC_WorkflowHandle handle, size_t stepIndex);

/**
 * @brief Get a read-only update manifest step handler (name)
 *
 * @param handle A workflow object handle.
 * @param stepIndex A step index.
 *
 * @return A step handler name.
 */
const char* workflow_peek_update_manifest_step_handler(ADUC_WorkflowHandle handle, size_t stepIndex);

/**
 * @brief Get a read-only handlerProperties string value.
 *
 * @param handle A workflow object handle.
 * @param propertyName
 *
 * @return A read-only string value of specified property in handlerProperties map.
 *         Returns NULL if specifed property doesnot exist, or not a 'string' type.
 */
const char*
workflow_peek_update_manifest_handler_properties_string(ADUC_WorkflowHandle handle, const char* propertyName);

/**
 * @brief Gets a reference step update manifest file at specified index.
 *
 * @param handle A workflow data object handle.
 * @param stepIndex A step index.
 * @param entity An output reference step update manifest file entity object.
 *               Caller must free the object with workflow_free_file_entity().
 * @return Returns true if succeeded.
 */
bool workflow_get_step_detached_manifest_file(ADUC_WorkflowHandle handle, size_t stepIndex, ADUC_FileEntity** entity);

/**
 * @brief Gets a serialized json string of the specified workflow's Update Manifest.
 *
 * @param handle A workflow data object handle.
 * @param pretty Whether to format the output string.
 * @return char* An output json string. Caller must free the string with workflow_free_string().
 */
char* workflow_get_serialized_update_manifest(ADUC_WorkflowHandle handle, bool pretty);

/**
 * @brief Gets the file path of the entity target update under the download work folder sandbox.
 *
 * @param workflowHandle The workflow handle.
 * @param entity The file entity.
 * @param outFilePath The resultant work folder file path to the file entity.
 * @return _Bool true if success
 * @remark Caller will own the STRING_HANDLE outFilePath and must call STRING_delete on it.
 */
_Bool workflow_get_entity_workfolder_filepath(
    ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* entity, STRING_HANDLE* outFilePath);

/**
 * @brief Gets boolean indicates whether to bypass duplicate workflow check, and always process the update.
 */
_Bool workflow_get_force_update(ADUC_WorkflowHandle workflowHandle);

/**
 * @brief Sets boolean indicates whether to bypass duplicate workflow check, and always process the update.
 *
 * @param handle The workflow handle.
 * @param forceUpdate Set to true to bypass duplicate workflow check.
 */
void workflow_set_force_update(ADUC_WorkflowHandle handle, bool forceUpdate);

EXTERN_C_END

#endif // ADUC_WORKFLOW_UTILS_H
