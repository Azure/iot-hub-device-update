/**
 * @file adu_core_json.h
 * @brief Defines types and methods required to parse JSON from the urn:azureiot:AzureDeviceUpdateCore:1 interface.
 *
 * Note that the types in this file must be kept in sync with the AzureDeviceUpdateCore interface.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_ADU_CORE_JSON_H
#define ADUC_ADU_CORE_JSON_H

#include <aduc/c_utils.h>
#include <stdbool.h>

#include <parson.h>

EXTERN_C_BEGIN

/**
 * @brief JSON field name for Action property
 */
#define ADUCITF_FIELDNAME_ACTION "action"

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
} ADUCITF_UpdateAction;

const char* ADUCITF_UpdateActionToString(ADUCITF_UpdateAction updateAction);

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
    ADUCITF_State_Idle = 0,
    ADUCITF_State_DownloadStarted = 1,
    ADUCITF_State_DownloadSucceeded = 2,
    ADUCITF_State_InstallStarted = 3,
    ADUCITF_State_InstallSucceeded = 4,
    ADUCITF_State_ApplyStarted = 5,

    ADUCITF_State_Failed = 255,
} ADUCITF_State;

const char* ADUCITF_StateToString(ADUCITF_State updateState);

/**
 * @brief JSON field name for ResultCode property.
 */
#define ADUCITF_FIELDNAME_RESULTCODE "resultCode"

/**
 * @brief JSON field name for ExtendedResultCode property.
 */
#define ADUCITF_FIELDNAME_EXTENDEDRESULTCODE "extendedResultCode"

/**
 * @brief JSON field name for installedCriteria property.
 */
#define ADUCITF_FIELDNAME_INSTALLEDCRITERIA "installedCriteria"

/**
 * @brief JSON field name for installedUpdateId property.
 */
#define ADUCITF_FIELDNAME_INSTALLEDUPDATEID "installedUpdateId"

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
 * @brief JSON field name for the updateManifest's hash held within the associated JWT
 */
#define ADUCITF_JWT_FIELDNAME_HASH "sha256"

/**
 * @brief JSON field name for the updateManifest's file entity's filename
 */
#define ADUCITF_FIELDNAME_FILENAME "fileName"

//
// Methods
//

struct json_value_t;
typedef struct json_value_t JSON_Value;

struct tagADUC_FileEntity;
struct tagADUC_UpdateId;

JSON_Value* ADUC_Json_GetRoot(const char* updateActionJsonString);

_Bool ADUC_Json_ValidateManifest(const JSON_Value* updateActionJson);

_Bool ADUC_Json_GetUpdateAction(const JSON_Value* updateActionJson, unsigned* updateAction);

_Bool ADUC_Json_GetInstalledCriteria(const JSON_Value* updateActionJson, char** installedCriteria);

_Bool ADUC_Json_GetUpdateType(const JSON_Value* updateActionJson, char** updateTypeStr);

_Bool ADUC_Json_GetUpdateId(const JSON_Value* updateActionJson, struct tagADUC_UpdateId** updateId);

_Bool ADUC_Json_GetFiles(
    const JSON_Value* updateActionJson, unsigned int* fileCount, struct tagADUC_FileEntity** files);

EXTERN_C_END

#endif // ADUC_ADU_CORE_JSON_H
