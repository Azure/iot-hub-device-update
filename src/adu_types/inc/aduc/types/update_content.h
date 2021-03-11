/**
 * @file update_content.h
 * @brief Defines types related to Update Content data.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_TYPES_UPDATE_CONTENT_H
#define ADUC_TYPES_UPDATE_CONTENT_H

#include "aduc/types/hash.h"
#include <stddef.h> // for size_t

EXTERN_C_BEGIN

/**
 * @brief JSON field name for state property.
 *
 */
#define ADUCITF_FIELDNAME_STATE "state"

/**
 * @brief JSON field name for Action property
 */
#define ADUCITF_FIELDNAME_ACTION "action"

/**
 * @brief JSON field name for Id property
 */
#define ADUCITF_FIELDNAME_ID "id"

/**
 * @brief JSON field name for RetryTimestamp property
 */
#define ADUCITF_FIELDNAME_RETRYTIMESTAMP "retryTimestamp"

/**
 * @brief JSON field name for Workflow Action property
 */
#define ADUCITF_FIELDNAME_WORKFLOW_DOT_ACTION "workflow.action"

/**
 * @brief JSON field name for Workflow Id property
 */
#define ADUCITF_FILEDNAME_WORKFLOW_DOT_ID "workflow.id"

/**
 * @brief JSON field name for LastInstallResult property.
 */
#define ADUCITF_FIELDNAME_LASTINSTALLRESULT "lastInstallResult"

/**
 * @brief JSON field name for UpdateInstallResult property.
 */
#define ADUCITF_FIELDNAME_UPDATEINSTALLRESULT "updateInstallResult"

/**
 * @brief JSON field name for stepResults property.
 */
#define ADUCITF_FIELDNAME_STEPRESULTS "stepResults"

/**
 * @brief JSON field name for ResultCode property.
 */
#define ADUCITF_FIELDNAME_RESULTCODE "resultCode"

/**
 * @brief JSON field name for ExtendedResultCode property.
 */
#define ADUCITF_FIELDNAME_EXTENDEDRESULTCODE "extendedResultCode"

/**
 * @brief JSON field name for ResultDetails property.
 */
#define ADUCITF_FIELDNAME_RESULTDETAILS "resultDetails"

/**
 * @brief JSON field name for installedCriteria property.
 */
#define ADUCITF_FIELDNAME_INSTALLEDCRITERIA "installedCriteria"

/**
 * @brief JSON field name for workflow property.
 */
#define ADUCITF_FIELDNAME_WORKFLOW "workflow"

/**
 * @brief JSON field name for installedUpdateId property.
 */
#define ADUCITF_FIELDNAME_INSTALLEDUPDATEID "installedUpdateId"

/**
 * @brief JSON field name for compatPropertyNames
 */
#define ADUCITF_FIELDNAME_COMPAT_PROPERTY_NAMES "compatPropertyNames"

/**
 * @brief JSON field name for DeviceProperties
 */
#define ADUCITF_FIELDNAME_DEVICEPROPERTIES "deviceProperties"

/**
 * @brief JSON field name for DeviceProperties manufacturer
 */
#define ADUCITF_FIELDNAME_DEVICEPROPERTIES_MANUFACTURER "manufacturer"

/**
 * @brief JSON field name for DeviceProperties model
 */
#define ADUCITF_FIELDNAME_DEVICEPROPERTIES_MODEL "model"

/**
 * @brief JSON field name for DeviceProperties interfaceId
 */
#define ADUCITF_FIELDNAME_DEVICEPROPERTIES_INTERFACEID "interfaceId"

/**
 * @brief JSON field name for DeviceProperties aduc version
 */
#define ADUCITF_FIELDNAME_DEVICEPROPERTIES_ADUC_VERSION "aduVer"

/**
 * @brief JSON field name for DeviceProperties DO version
 */
#define ADUCITF_FIELDNAME_DEVICEPROPERTIES_DO_VERSION "doVer"

/**
 * @brief JSON field name for the updateManifest
 */
#define ADUCITF_FIELDNAME_UPDATEMANIFEST "updateManifest"
/**
 * @brief JSON field name for the updateManifestSignature
 */
#define ADUCITF_FIELDNAME_UPDATEMANIFESTSIGNATURE "updateManifestSignature"

/**
 * @brief JSON field name for File Urls
 */
#define ADUCITF_FIELDNAME_FILE_URLS "fileUrls"

//
// Update Manifest Field Values
//

/**
 * @brief JSON field name for the updateManifest's updateType
 */
#define ADUCITF_FIELDNAME_UPDATETYPE "updateType"

/**
 * @brief JSON field name for the updateManifest's UpdateId
 */
#define ADUCITF_FIELDNAME_UPDATEID "updateId"

/**
 * @brief JSON field name for the UpdateManifest's provider
 */
#define ADUCITF_FIELDNAME_PROVIDER "provider"

/**
 * @brief JSON field name for the updateManifest's name
 */
#define ADUCITF_FIELDNAME_NAME "name"

/**
 * @brief JSON field name for the updateManifest's version
 */
#define ADUCITF_FIELDNAME_VERSION "version"

/**
 * @brief JSON field name for Files property in the updateManifest.
 */
#define ADUCITF_FIELDNAME_FILES "files"

/**
 * @brief JSON field name for Hashes
 */
#define ADUCITF_FIELDNAME_HASHES "hashes"

/**
 * @brief JSON field name for file size (in bytes).
 */
#define ADUCITF_FIELDNAME_SIZEINBYTES "sizeInBytes"

/**
 * @brief JSON field name for the updateManifest's hash held within the associated JWT
 */
#define ADUCITF_JWT_FIELDNAME_HASH "sha256"

/**
 * @brief JSON field name for the updateManifest's file entity's filename
 */
#define ADUCITF_FIELDNAME_FILENAME "fileName"

/**
 * @brief JSON field name for the updateManifest's file entity's arguments
 */
#define ADUCITF_FIELDNAME_ARGUMENTS "arguments"

//
// UpdateAction
//

// clang-format off

/**
 * @brief UpdateAction values sent to the agent from the service.
 *
 * Values associated with UpdateAction in AzureDeviceUpdateCore interface.
 */
typedef enum tagADUCITF_UpdateAction
{
    // PPR+ agents no longer support the following 3 update actions:
    ADUCITF_UpdateAction_Invalid_Download = 0, ///< Request to download an update. For legacy cloud-driven orchestration. Not supported.
    ADUCITF_UpdateAction_Invalid_Install = 1, ///< Request to install an update. For legacy cloud-driven orchestration. Not supported.
    ADUCITF_UpdateAction_Invalid_Apply = 2, ///< Request to do a post-install apply step for reboots, etc. For legacy cloud-driven orchestration. Not supported.

    // Agent-orchestrated update workflow Update Actions
    ADUCITF_UpdateAction_ProcessDeployment = 3, ///< Request to process an update workflow deployment via client-driven orchestration.
    ADUCITF_UpdateAction_Cancel = 255, ///< Request to cancel an ongoing update workflow deployment.

    ADUCITF_UpdateAction_Undefined = -1 ///< An undefined update action.
} ADUCITF_UpdateAction;

/**
 * @brief WorkflowStep values
 *
 * Requested intraphase transitions representing workflow steps in the Agent.
 * These values do have no relation to protocol values and are for internal
 * workflow state machine only, but are in order they would occur While
 * processing the update workflow.
 */
typedef enum tagADUCITF_WorkflowStep
{
    ADUCITF_WorkflowStep_Undefined = 0, ///< The undefined worfklow step.
    ADUCITF_WorkflowStep_ProcessDeployment = 1, ///< Step that reports DeploymentInProgress ACK
    ADUCITF_WorkflowStep_Download = 2, ///< Step where it starts download operation
    ADUCITF_WorkflowStep_Install = 3, ///< Step where it starts install operation
    ADUCITF_WorkflowStep_Apply = 4, ///< Step where it starts apply operation
} ADUCITF_WorkflowStep;

//
// UpdateState
//

/**
 * @brief Agent-orchestration internal state machine states.
 *
 * Currently, Idle, DeploymentInProgress, and Failed states report the "state"
 * value of the enum on the wire as per the client-server protocol.
 */
typedef enum tagADUCITF_State
{
    ADUCITF_State_None = -1, ///< The state for when workflow data opaque, internal data is invalid.
    ADUCITF_State_Idle = 0, ///< The start state and the state for reporting update deployment success.
    ADUCITF_State_DownloadStarted = 1, ///< The state for when a download operation is in progress.
    ADUCITF_State_DownloadSucceeded = 2, ///< The state for when a download operation has completed with success.
    ADUCITF_State_InstallStarted = 3, ///< The state for when an install operation is in progress.
    ADUCITF_State_InstallSucceeded = 4, ///< The state for when an install operation has completed with success.
    ADUCITF_State_ApplyStarted = 5, ///< The state for when an apply operation is in progress.
    ADUCITF_State_DeploymentInProgress = 6, ///< The state for reporting the acknowledgement of ProcessDeployment update action.
    ADUCITF_State_Failed = 255, ///< The state for when a deployment has failed and for reporting the failure.
} ADUCITF_State;

// clang-format on

/**
 * @brief Describes an update identity.
 */
typedef struct tagADUC_UpdateId
{
    char* Provider; /**< Provider for the update */
    char* Name; /**< Name for the update */
    char* Version; /**< Version for the update*/
} ADUC_UpdateId;

/**
 * @brief Describes a specific file to download.
 */
typedef struct tagADUC_FileEntity
{
    char* FileId; /**< Id for the file */
    char* DownloadUri; /**< The URI of the file to download. */
    ADUC_Hash* Hash; /**< Array of ADUC_Hashes containing the hash options for the file*/
    size_t HashCount; /**< Total number of hashes in the array of hashes */
    char* TargetFilename; /**< File name to store content in DownloadUri to. */
    char* Arguments; //**< Arguments associate with this file. */
    size_t SizeInBytes; /**< File size. */
} ADUC_FileEntity;

/**
 * @brief Describes a specific file to download.
 */
typedef struct tagADUC_FileUrl
{
    char* FileId; /**< Id for the file */
    char* DownloadUri; /**< The URI of the file to download. */
} ADUC_FileUrl;

//
// ADUC_UpdateId Helper Functions
//

/**
 * @brief Allocates and sets the UpdateId fields
 * @param provider the provider for the UpdateId
 * @param name the name for the UpdateId
 * @param version the version for the UpdateId
 *
 * @returns An UpdateId on success, NULL on failure
 */
ADUC_UpdateId* ADUC_UpdateId_AllocAndInit(const char* provider, const char* name, const char* version);

/**
 * @brief Checks if the UpdateId is valid
 * @param updateId updateId to check
 * @returns True if it is valid, false if not
 */
_Bool ADUC_IsValidUpdateId(const ADUC_UpdateId* updateId);

/**
 * @brief Free the UpdateId and its content.
 * @param updateId a pointer to an updateId struct to be freed
 */
void ADUC_UpdateId_UninitAndFree(ADUC_UpdateId* updateId);

/**
 * @brief Convert UpdateState to string representation.
 *
 * @param updateState State to convert.
 * @return const char* String representation.
 */
const char* ADUCITF_StateToString(ADUCITF_State updateState);

/**
 * @brief Convert UpdateAction to string representation.
 *
 * @param updateAction Action to convert.
 * @return const char* String representation.
 */
const char* ADUCITF_UpdateActionToString(ADUCITF_UpdateAction updateAction);

EXTERN_C_END

#endif // ADUC_TYPES_UPDATE_CONTENT_H
