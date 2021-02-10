/**
 * @file adu_core_export_helpers.h
 * @brief Provides set of helpers for creating objects defined in adu_core_exports.h
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_ADU_CORE_EXPORT_HELPERS_H
#define ADUC_ADU_CORE_EXPORT_HELPERS_H

#include <aduc/c_utils.h>

#include <stdbool.h>

#include "aduc/adu_core_exports.h"
#include "aduc/agent_workflow.h"

EXTERN_C_BEGIN

/**
 * @brief Frees each FileEntity's members and the ADUC_FileEntity array
 * @param filecount the total number of files held within files
 * @param files a pointer to an array of ADUC_FileEntity objects
 */
void ADUC_FileEntityArray_Free(unsigned int fileCount, ADUC_FileEntity* files);

//
// ADUC_Hash Helper Functions
//

/**
 * @brief Allocates the memory for the ADUC_Hash struct member values
 * @param hash A pointer to an ADUC_Hash struct whose member values will be allocated
 * @param hashValue The value of the hash
 * @param hashType The type of the hash
 * @returns True if successfully allocated, False if failure
 */
_Bool ADUC_Hash_Init(ADUC_Hash* hash, const char* hashValue, const char* hashType);

/**
 * @brief Free the ADUC_Hash struct members
 * @param hash a pointer to an ADUC_Hash
 */
void ADUC_Hash_UnInit(ADUC_Hash* hash);

/**
 * @brief Frees an array of ADUC_Hashes of size @p hashCount
 * @param hashCount the size of @p hashArray
 * @param hashes a pointer to an array of ADUC_Hash structs
 */
void ADUC_Hash_FreeArray(size_t hashCount, ADUC_Hash* hashArray);

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
 * @brief Returns a serialized, valid JSON string of the updateId
 * @details Caller is responsible for using the Parson json_free_serialized_string or a call to free to free the allocated string
 * @param updateId updateId to serialize to a json compatible string
 * @returns a pointer to a valid JSON string of the updateId
 */
char* ADUC_UpdateIdToJsonString(const ADUC_UpdateId* updateId);

/**
 * @brief Checks if the UpdateId is valid
 * @param updateId updateId to check
 * @returns True if it is valid, false if not
 */
_Bool ADUC_IsValidUpdateId(const ADUC_UpdateId* updateId);

/**
 * @brief Free the UpdateId
 * @param updateid a pointer to an updateId struct to be freed
 */
void ADUC_UpdateId_Free(ADUC_UpdateId* updateId);

//
// ADUC_PrepareInfo helpers.
//
/**
 * @brief Initializes the fields of an PrepareInfo object.
 *
 * @param info PrepareInfo object to set.
 * @param Workflow data
 * @return _Bool True on success.
 */

_Bool ADUC_PrepareInfo_Init(ADUC_PrepareInfo* info, const ADUC_WorkflowData* workflowData);

/**
 * @brief Free PrepareInfo object members.
 *
 * @param info PrepareInfo object to clear.
 */
void ADUC_PrepareInfo_UnInit(ADUC_PrepareInfo* info);

//
// ADUC_DownloadInfo helpers.
//

/**
 * @brief Initializes the fields of an DownloadInfo object.
 *
 * @param info DownloadInfo object to set.
 * @param updateActionJson JSON containing UpdateAction information.
 * @param workFolder Path to work folder.
 * @param progressCallback Method to call when download progress information is received.
 * @return _Bool True on success.
 */
_Bool ADUC_DownloadInfo_Init(
    ADUC_DownloadInfo* info,
    const JSON_Value* updateActionJson,
    const char* workFolder,
    ADUC_DownloadProgressCallback progressCallback);

/**
 * @brief Free DownloadInfo object members.
 *
 * @param info DownloadInfo object to clear.
 */
void ADUC_DownloadInfo_UnInit(ADUC_DownloadInfo* info);

//
// ADUC_InstallInfo helpers.
//

/**
 * @brief Initializes the fields of an InstallInfo object.
 *
 * @param info InstallInfo object to set.
 * @param workFolder Path to work folder.
 * @return _Bool True on success.
 */
_Bool ADUC_InstallInfo_Init(ADUC_InstallInfo* info, const char* workFolder);

/**
 * @brief Free InstallInfo object members.
 *
 * @param info InstallInfo object to clear.
 */
void ADUC_InstallInfo_UnInit(ADUC_InstallInfo* info);

//
// ADUC_ApplyInfo methods.
//

/**
 * @brief Initializes the fields of an ApplyInfo object.
 *
 * @param info Object to set.
 * @param workFolder Path to work folder.
 * @return _Bool True on success.
 */
_Bool ADUC_ApplyInfo_Init(ADUC_ApplyInfo* info, const char* workFolder);

/**
 * @brief Free ApplyInfo object members.
 *
 * @param info ApplyInfo object to clear.
 */
void ADUC_ApplyInfo_UnInit(ADUC_ApplyInfo* info);

//
// Register/Unregister methods.
//

/**
 * @brief Call Unregister method to indicate startup.
 *
 * @param registerData RegisterData object.
 * @param argc Count of arguments in @p argv
 * @param argv Command line parameters.
 */
ADUC_Result ADUC_MethodCall_Register(ADUC_RegisterData* registerData, unsigned int argc, const char** argv);

/**
 * @brief Call Unregister method to indicate shutdown.
 *
 * @param registerData RegisterData object.
 */
void ADUC_MethodCall_Unregister(const ADUC_RegisterData* registerData);

//
// ADU Core Interface update action methods.
//

typedef struct tagADUC_MethodCall_Data
{
    ADUC_WorkCompletionData WorkCompletionData;
    ADUC_WorkflowData* WorkflowData;

    union tagMethodSpecificData {
        ADUC_DownloadInfo* DownloadInfo;
        ADUC_InstallInfo* InstallInfo;
        ADUC_ApplyInfo* ApplyInfo;
    } MethodSpecificData;
} ADUC_MethodCall_Data;

ADUC_Result ADUC_MethodCall_Prepare(const ADUC_WorkflowData* workflowData);

ADUC_Result ADUC_MethodCall_Download(ADUC_MethodCall_Data* methodCallData);
void ADUC_MethodCall_Download_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

ADUC_Result ADUC_MethodCall_Install(ADUC_MethodCall_Data* methodCallData);
void ADUC_MethodCall_Install_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

ADUC_Result ADUC_MethodCall_Apply(ADUC_MethodCall_Data* methodCallData);
void ADUC_MethodCall_Apply_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

void ADUC_MethodCall_Cancel(const ADUC_WorkflowData* workflowData);

ADUC_Result ADUC_MethodCall_IsInstalled(const ADUC_WorkflowData* workflowData);

//
// State transition
//

void ADUC_SetUpdateState(ADUC_WorkflowData* workflowData, ADUCITF_State updateState);
void ADUC_SetUpdateStateWithResult(ADUC_WorkflowData* workflowData, ADUCITF_State updateState, ADUC_Result result);
void ADUC_SetInstalledUpdateIdAndGoToIdle(ADUC_WorkflowData* workflowData, const ADUC_UpdateId* updateId);

//
// Reboot system
//

int ADUC_MethodCall_RebootSystem();

//
// Restart the ADU Agent.
//

int ADUC_MethodCall_RestartAgent();

EXTERN_C_END

#endif // ADUC_ADU_CORE_EXPORT_HELPERS_H
