/**
 * @file microsoft_delta_download_handler_utils.h
 * @brief The microsoft delta download handler helper function prototypes.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef MICROSOFT_DELTA_DOWNLOAD_HANDLER_UTILS_H
#define MICROSOFT_DELTA_DOWNLOAD_HANDLER_UTILS_H

#include <aduc/c_utils.h> // EXTERN_C_BEGIN, EXTERN_C_END
#include <aduc/types/adu_core.h> // ADUC_Result_*
#include <aduc/types/update_content.h> // ADUC_RelatedFile
#include <aduc/types/workflow.h> // ADUC_WorkflowHandle
#include <azure_c_shared_utility/strings.h> // STRING_*

EXTERN_C_BEGIN

typedef ADUC_Result (*ProcessDeltaUpdateFn)(
    const char* sourceUpdateFilePath, const char* deltaUpdateFilePath, const char* targetUpdateFilePath);

typedef ADUC_Result (*DownloadDeltaUpdateFn)(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_RelatedFile* relatedFile);

/**
 * @brief Processes a related file of an update for delta download handling.
 *
 * @param workflowHandle The workflow handle.
 * @param relatedFile The related file.
 * @param payloadFilePath The payload file path.
 * @param updateCacheBasePath The update cache base path. Use NULL for default.
 * @param processDeltaUpdateFn The function to call to process delta updates.
 * @param downloadDeltaUpdateFn The function to call to download the delta update.
 * @return ADUC_Result The result.
 * @details Returns ADUC_Result_Success_Cache_Miss when delta relatedFile source update not found in cache.
 */
ADUC_Result MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
    const ADUC_WorkflowHandle workflowHandle,
    const ADUC_RelatedFile* relatedFile,
    const char* payloadFilePath,
    const char* updateCacheBasePath,
    ProcessDeltaUpdateFn processDeltaUpdateFn,
    DownloadDeltaUpdateFn downloadDeltaUpdateFn);

/**
 * @brief Looks up the source update in the source update cache and outputs the path to it, if it exists.
 *
 * @param[in] workflowHandle The workflow handle.
 * @param[in] relatedFile The related file with the relationship to the source update.
 * @param[in] updateCacheBasePath The update cache base path. Use NULL for default.
 * @param[out] outPathHandle The STRING_HANDLE to hold the resultant filepath to the source update.
 * @return ADUC_Result The result.
 * @details Returns ResultCode of ADUC_Result_Success_Cache_Miss on cache miss, ADUC_Result_Success on cache hit, and ADUC_Result_Failure otherwise.
 */
ADUC_Result MicrosoftDeltaDownloadHandlerUtils_LookupSourceUpdateCachePath(
    const ADUC_WorkflowHandle workflowHandle,
    const ADUC_RelatedFile* relatedFile,
    const char* updateCacheBasePath,
    STRING_HANDLE* outPathHandle);

/**
 * @brief Gets the source hash and hash algorithm from the related file.
 *
 * @param[in] relatedFile The related file.
 * @param[out] outHash The output parameter for the hash.
 * @param[out] outAlg The output parameter for the alg.
 * @return ADUC_Result The result.
 */
ADUC_Result MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(
    const ADUC_RelatedFile* relatedFile, STRING_HANDLE* outHash, STRING_HANDLE* outAlg);

/**
 * @brief Downloads a delta update related file.
 *
 * @param workflowHandle The workflow handle.
 * @param relatedFile The related file.
 * @return ADUC_Result The result.
 */
ADUC_Result MicrosoftDeltaDownloadHandlerUtils_DownloadDeltaUpdate(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_RelatedFile* relatedFile);

/**
 * @brief Gets the file path to the delta update downloaded in the download sandbox work folder.
 *
 * @param[in] workflowHandle The workflow handle.
 * @param[in] relatedFile The related file with the relationship to the source update.
 * @param[out] outPathHandle The STRING_HANDLE to hold the resultant filepath to the delta update.
 * @return ADUC_Result The result.
 */
ADUC_Result MicrosoftDeltaDownloadHandlerUtils_GetDeltaUpdateDownloadSandboxPath(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_RelatedFile* relatedFile, STRING_HANDLE* outPathHandle);

/**
 * @brief Creates a target update from the source and delta updates.
 *
 * @param sourceUpdateFilePath The source update path.
 * @param deltaUpdateFilePath The delta update path.
 * @param targetUpdatePath The target update path.
 * @return ADUC_Result The result.
 */
ADUC_Result MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate(
    const char* sourceUpdateFilePath, const char* deltaUpdateFilePath, const char* targetUpdateFilePath);

EXTERN_C_END

#endif // MICROSOFT_DELTA_DOWNLOAD_HANDLER_UTILS_H
