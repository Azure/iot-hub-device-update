/**
 * @file microsoft_delta_download_handler_utils.c
 * @brief The Microsoft delta download handler helper function implementations.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/microsoft_delta_download_handler_utils.h"
#include "aduc/c_utils.h" // EXTERN_C_BEGIN, EXTERN_C_END
#include "aduc/source_update_cache.h" // ADUC_SourceUpdateCache_Lookup
#include <aduc/extension_manager.h> // ExtensionManager_Download
#include <aduc/string_c_utils.h> // IsNullOrEmpty
#include <aduc/workflow_utils.h> // workflow_*
#include <azure_c_shared_utility/crt_abstractions.h> // mallocAndStrcpy_s
#include <azure_c_shared_utility/strings.h> // STRING_*
#include <stdlib.h> // free

/* external linkage */
extern ExtensionManager_Download_Options Default_ExtensionManager_Download_Options;

EXTERN_C_BEGIN

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
    DownloadDeltaUpdateFn downloadDeltaUpdateFn)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };
    STRING_HANDLE sourceUpdatePathHandle = NULL;
    STRING_HANDLE deltaUpdatePathHandle = NULL;

    if (workflowHandle == NULL || relatedFile == NULL || payloadFilePath == NULL || processDeltaUpdateFn == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_DDH_BAD_ARGS;
        return result;
    }

    //
    // See if source full update is in the update cache.
    //
    result = MicrosoftDeltaDownloadHandlerUtils_LookupSourceUpdateCachePath(
        workflowHandle, relatedFile, updateCacheBasePath, &sourceUpdatePathHandle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    if (result.ResultCode == ADUC_Result_Success_Cache_Miss)
    {
        goto done;
    }

    Log_Debug("cached source update found at '%s'. Downloading delta update...", STRING_c_str(sourceUpdatePathHandle));

    //
    // Download the delta update file.
    //
    result = downloadDeltaUpdateFn(workflowHandle, relatedFile);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("DeltaUpdate download failed, erc 0x%08x.", result.ExtendedResultCode);
        goto done;
    }

    //
    // Get the path to the downloaded delta update file in the sandbox.
    //
    result = MicrosoftDeltaDownloadHandlerUtils_GetDeltaUpdateDownloadSandboxPath(
        workflowHandle, relatedFile, &deltaUpdatePathHandle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("get delta update sandbox path, erc 0x%08x.", result.ExtendedResultCode);
        goto done;
    }

    Log_Debug("Processing delta update at '%s'...", STRING_c_str(deltaUpdatePathHandle));

    //
    // Use the delta processor to produce the full target update from the source update and delta update.
    //
    const char* srcPath = STRING_c_str(sourceUpdatePathHandle);
    const char* deltaPath = STRING_c_str(deltaUpdatePathHandle);
    result = processDeltaUpdateFn(srcPath, deltaPath, payloadFilePath);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("processing delta update failed, ERC 0x%08x", result.ExtendedResultCode);
        goto done;
    }

    result.ResultCode = ADUC_Result_Success;

done:

    STRING_delete(deltaUpdatePathHandle);
    STRING_delete(sourceUpdatePathHandle);

    return result;
}

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
    STRING_HANDLE* outPathHandle)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };
    STRING_HANDLE sourceUpdatePath = NULL;
    ADUC_UpdateId* updateId = NULL;

    STRING_HANDLE sourceUpdateHash = NULL;
    STRING_HANDLE sourceUpdateAlg = NULL;

    result =
        MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(relatedFile, &sourceUpdateHash, &sourceUpdateAlg);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("get source update properties failed, erc 0x%08x", result.ExtendedResultCode);
        goto done;
    }

    result = workflow_get_expected_update_id(workflowHandle, &updateId);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("get updateId, erc 0x%08x", result.ExtendedResultCode);
        goto done;
    }

    Log_Debug("Get SourceUpdatePath for relatedFile '%s'", relatedFile->FileName);

    result = ADUC_SourceUpdateCache_Lookup(
        updateId->Provider,
        STRING_c_str(sourceUpdateHash),
        STRING_c_str(sourceUpdateAlg),
        updateCacheBasePath,
        &sourceUpdatePath);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("source lookup failed, erc 0x%08x", result.ExtendedResultCode);
        goto done;
    }

    if (result.ResultCode == ADUC_Result_Success_Cache_Miss)
    {
        Log_Warn("source update cache miss");
        goto done;
    }

    *outPathHandle = sourceUpdatePath;
    sourceUpdatePath = NULL;

    result.ResultCode = ADUC_Result_Success;

done:
    STRING_delete(sourceUpdateHash);
    STRING_delete(sourceUpdateAlg);
    workflow_free_update_id(updateId);
    free(sourceUpdatePath);

    return result;
}

/**
 * @brief Gets the source hash and hash algorithm from the related file.
 *
 * @param[in] relatedFile The related file.
 * @param[out] outHash The output parameter for the hash.
 * @param[out] outAlg The output parameter for the alg.
 * @return ADUC_Result The result.
 */
ADUC_Result MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(
    const ADUC_RelatedFile* relatedFile, STRING_HANDLE* outHash, STRING_HANDLE* outAlg)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure, .ExtendedResultCode = 0 };

    const char* sourceHash = NULL;
    const char* sourceAlg = NULL;

    STRING_HANDLE tempHash = NULL;
    STRING_HANDLE tempAlg = NULL;

    if (relatedFile == NULL || outHash == NULL || outAlg == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_DDH_BAD_ARGS;
        return result;
    }

    for (int indexProperty = 0; indexProperty < relatedFile->PropertiesCount; ++indexProperty)
    {
        const char* propertyName = relatedFile->Properties[indexProperty].Name;
        if (strcmp(propertyName, "microsoft.sourceFileHash") == 0)
        {
            sourceHash = relatedFile->Properties[indexProperty].Value;
        }
        else if (strcmp(propertyName, "microsoft.sourceFileHashAlgorithm") == 0)
        {
            sourceAlg = relatedFile->Properties[indexProperty].Value;
        }
    }

    if (IsNullOrEmpty(sourceHash) || IsNullOrEmpty(sourceAlg))
    {
        Log_Error("Missing microsoft.sourceFileHash or microsoft.sourceFileHashAlgorithm relatedFile property.");

        result.ExtendedResultCode = ADUC_ERC_DDH_RELATEDFILE_BAD_OR_MISSING_HASH_PROPERTIES;

        goto done;
    }

    // Only set outputs once all allocations succeeded
    tempHash = STRING_construct(sourceHash);
    tempAlg = STRING_construct(sourceAlg);

    if (tempHash == NULL || tempAlg == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    // now can transfer
    *outHash = tempHash;
    tempHash = NULL;

    *outAlg = tempAlg;
    tempAlg = NULL;

    result.ResultCode = ADUC_Result_Success;

done:
    STRING_delete(tempHash);
    STRING_delete(tempAlg);

    return result;
}

/**
 * @brief Downloads a delta update related file.
 *
 * @param workflowHandle The workflow handle.
 * @param relatedFile The related file.
 * @return ADUC_Result The result.
 */
ADUC_Result MicrosoftDeltaDownloadHandlerUtils_DownloadDeltaUpdate(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_RelatedFile* relatedFile)
{
    Log_Debug("Try download delta update from '%s'", relatedFile->DownloadUri);

    ADUC_FileEntity deltaUpdateFileEntity = {
        .DownloadUri = relatedFile->DownloadUri,
        .FileId = relatedFile->FileId,
        .Hash = relatedFile->Hash,
        .HashCount = relatedFile->HashCount,
        .SizeInBytes = relatedFile->SizeInBytes,
        .TargetFilename = relatedFile->FileName,
    };

    return ExtensionManager_Download(
        &deltaUpdateFileEntity,
        workflowHandle,
        &Default_ExtensionManager_Download_Options,
        NULL /* downloadProgressCallback */);
}

/**
 * @brief Gets the file path to the delta update downloaded in the download sandbox work folder.
 *
 * @param[in] workflowHandle The workflow handle.
 * @param[in] relatedFile The related file with the relationship to the source update.
 * @param[out] outPathHandle The STRING_HANDLE to hold the resultant filepath to the delta update.
 * @return ADUC_Result The result.
 */
ADUC_Result MicrosoftDeltaDownloadHandlerUtils_GetDeltaUpdateDownloadSandboxPath(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_RelatedFile* relatedFile, STRING_HANDLE* outPathHandle)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };
    STRING_HANDLE sandboxPathHandle = NULL;

    char* workFolder = workflow_get_workfolder(workflowHandle);

    if (workFolder == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    sandboxPathHandle = STRING_new();

    if (sandboxPathHandle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    if (STRING_sprintf(sandboxPathHandle, "%s/%s", workFolder, relatedFile->FileName) != 0)
    {
        result.ExtendedResultCode = ADUC_ERC_DDH_MAKE_DELTA_UPDATE_PATH;
        goto done;
    }

    *outPathHandle = sandboxPathHandle;
    sandboxPathHandle = NULL;

    result.ResultCode = ADUC_Result_Success;

done:
    free(workFolder);
    STRING_delete(sandboxPathHandle);

    return result;
}

EXTERN_C_END
