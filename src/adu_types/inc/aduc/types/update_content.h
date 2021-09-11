/**
 * @file update_content.h
 * @brief Defines types related to Update Content data.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 */
#ifndef ADUC_TYPES_UPDATE_CONTENT_H
#define ADUC_TYPES_UPDATE_CONTENT_H

#include "aduc/types/hash.h"
#include <stddef.h> // for size_t

EXTERN_C_BEGIN

/**
 * @brief JSON field name for Action property
 */
#define ADUCITF_FIELDNAME_ACTION "action"

/**
 * @brief JSON field name for Workflow Action property
 */
#define ADUCITF_FIELDNAME_WORKFLOW_DOT_ACTION "workflow.action"

/**
 * @brief JSON field name for Workflow Id property
 */
#define ADUCITF_FILEDNAME_WORKFLOW_DOT_ID "workflow.id"

//
// UpdateAction
//

/**
 * @brief UpdateAction values.
 *
 * Values associated with UpdateAction in AzureDeviceUpdateCore interface.
 */
typedef enum tagADUCITF_UpdateAction
{
    ADUCITF_UpdateAction_Download = 0,
    ADUCITF_UpdateAction_Install = 1,
    ADUCITF_UpdateAction_Apply = 2,

    ADUCITF_UpdateAction_Cancel = 255,

    ADUCITF_UpdateAction_Undefined = -1
} ADUCITF_UpdateAction;

//
// UpdateState
//

/**
 * @brief JSON field name for state property.
 *
 */
#define ADUCITF_FIELDNAME_STATE "state"

/**
 * @brief UpdateState values.
 *
 * Values sent back to hub to indicate device agent state.
 */
typedef enum tagADUCITF_State
{
    ADUCITF_State_None = -1,

    ADUCITF_State_Idle = 0,
    ADUCITF_State_DownloadStarted = 1,
    ADUCITF_State_DownloadSucceeded = 2,
    ADUCITF_State_InstallStarted = 3,
    ADUCITF_State_InstallSucceeded = 4,
    ADUCITF_State_ApplyStarted = 5,

    ADUCITF_State_Failed = 255,
} ADUCITF_State;

/**
 * @brief JSON field name for LastInstallResult property.
 */
#define ADUCITF_FIELDNAME_LASTINSTALLRESULT "lastInstallResult"

/**
 * @brief JSON field name for UpdateInstallResult property.
 */
#define ADUCITF_FIELDNAME_UPDATEINSTALLRESULT "updateInstallResult"

/**
 * @brief JSON field name for BundledUpdates property.
 */
#define ADUCITF_FIELDNAME_BUNDLEDUPDATES "bundledUpdates"

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
 * @param updateid a pointer to an updateId struct to be freed
 */
void ADUC_UpdateId_UninitAndFree(ADUC_UpdateId* updateId);

EXTERN_C_END

#endif // ADUC_TYPES_UPDATE_CONTENT_H