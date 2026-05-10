/**
 * @file manifest_parser.c
 * @brief Implementation of the ADU Gen2 deployment manifest parser.
 *
 * Parses Gen1-compatible JSON deployment manifests using the Parson library.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/manifest_parser.h"

#include <parson.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Error codes for manifest parsing (facility=WORKFLOW, category=CONFIG) */
#define MANIFEST_PARSE_ERROR ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_CONFIG, 1)
#define MANIFEST_ALLOC_ERROR ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_RESOURCE, 1)
#define MANIFEST_INVALID_ERROR ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_CONFIG, 2)

/**
 * @brief Split a comma-separated string into an array of trimmed tokens.
 * @param csv  The comma-separated string (may be NULL).
 * @param outCount  Receives the number of tokens.
 * @return Heap-allocated array of strdup'd strings, or NULL if csv is NULL/empty.
 */
static char** split_csv(const char* csv, size_t* outCount)
{
    *outCount = 0;
    if (csv == NULL || csv[0] == '\0')
    {
        return NULL;
    }

    /* Count commas to estimate token count */
    size_t capacity = 1;
    for (const char* p = csv; *p; p++)
    {
        if (*p == ',')
        {
            capacity++;
        }
    }

    char** tokens = (char**)calloc(capacity, sizeof(char*));
    if (tokens == NULL)
    {
        return NULL;
    }

    size_t count = 0;
    const char* start = csv;
    while (*start)
    {
        /* Skip leading whitespace */
        while (*start == ' ' || *start == '\t')
        {
            start++;
        }

        const char* end = start;
        while (*end && *end != ',')
        {
            end++;
        }

        /* Trim trailing whitespace */
        const char* trimEnd = end;
        while (trimEnd > start && (*(trimEnd - 1) == ' ' || *(trimEnd - 1) == '\t'))
        {
            trimEnd--;
        }

        if (trimEnd > start)
        {
            size_t len = (size_t)(trimEnd - start);
            char* token = (char*)malloc(len + 1);
            if (token == NULL)
            {
                for (size_t i = 0; i < count; i++)
                {
                    free(tokens[i]);
                }
                free(tokens);
                *outCount = 0;
                return NULL;
            }
            memcpy(token, start, len);
            token[len] = '\0';
            tokens[count++] = token;
        }

        if (*end == ',')
        {
            end++;
        }
        start = end;
    }

    *outCount = count;
    return tokens;
}

/**
 * @brief Safe string copy into a fixed-size buffer.
 */
static void safe_strcpy(char* dest, size_t destSize, const char* src)
{
    if (src == NULL)
    {
        dest[0] = '\0';
        return;
    }
    size_t len = strlen(src);
    if (len >= destSize)
    {
        len = destSize - 1;
    }
    memcpy(dest, src, len);
    dest[len] = '\0';
}

/**
 * @brief Parse the "files" object from the manifest JSON.
 *
 * Iterates over the keys of the "files" object, extracting fileName,
 * sizeInBytes, and hashes.sha256 for each entry.
 */
static ADUC_Result2 parse_files(JSON_Object* filesObj, ADUC_StepFile** outFiles, size_t* outCount)
{
    size_t count = json_object_get_count(filesObj);
    if (count == 0)
    {
        *outFiles = NULL;
        *outCount = 0;
        return ADUC_RESULT2_SUCCESS;
    }

    ADUC_StepFile* files = (ADUC_StepFile*)calloc(count, sizeof(ADUC_StepFile));
    if (files == NULL)
    {
        return MANIFEST_ALLOC_ERROR;
    }

    for (size_t i = 0; i < count; i++)
    {
        const char* fileId = json_object_get_name(filesObj, i);
        JSON_Object* fileObj = json_value_get_object(json_object_get_value_at(filesObj, i));

        if (fileId == NULL || fileObj == NULL)
        {
            free(files);
            return MANIFEST_INVALID_ERROR;
        }

        const char* fileName = json_object_get_string(fileObj, "fileName");
        double sizeInBytes = json_object_get_number(fileObj, "sizeInBytes");
        JSON_Object* hashesObj = json_object_get_object(fileObj, "hashes");
        const char* sha256 = hashesObj ? json_object_get_string(hashesObj, "sha256") : NULL;

        files[i].id = strdup(fileId);
        files[i].name = fileName ? strdup(fileName) : NULL;
        files[i].path = NULL; /* Set after download */
        files[i].size = (uint64_t)sizeInBytes;
        files[i].sha256 = sha256 ? strdup(sha256) : NULL;

        if (files[i].id == NULL)
        {
            /* Cleanup on allocation failure */
            for (size_t j = 0; j <= i; j++)
            {
                free((void*)files[j].id);
                free((void*)files[j].name);
                free((void*)files[j].sha256);
            }
            free(files);
            return MANIFEST_ALLOC_ERROR;
        }
    }

    *outFiles = files;
    *outCount = count;
    return ADUC_RESULT2_SUCCESS;
}

/**
 * @brief Find a file in the global file table by ID.
 */
static const ADUC_StepFile* find_file_by_id(const ADUC_StepFile* files, size_t fileCount, const char* fileId)
{
    if (fileId == NULL)
    {
        return NULL;
    }
    for (size_t i = 0; i < fileCount; i++)
    {
        if (files[i].id != NULL && strcmp(files[i].id, fileId) == 0)
        {
            return &files[i];
        }
    }
    return NULL;
}

/**
 * @brief Parse the "instructions.steps" array from the manifest JSON.
 *
 * For each step: extracts type, handler, handlerProperties (serialized back
 * to JSON), files array, and installedCriteria. Reference-type steps are
 * skipped (TODO: handle child manifests).
 */
static ADUC_Result2 parse_steps(
    JSON_Array* stepsArray,
    const ADUC_StepFile* globalFiles,
    size_t globalFileCount,
    ADUC_DeploymentStep** outSteps,
    size_t* outCount)
{
    size_t totalSteps = json_array_get_count(stepsArray);
    if (totalSteps == 0)
    {
        *outSteps = NULL;
        *outCount = 0;
        return ADUC_RESULT2_SUCCESS;
    }

    /* Allocate max possible (some reference steps may be skipped) */
    ADUC_DeploymentStep* steps = (ADUC_DeploymentStep*)calloc(totalSteps, sizeof(ADUC_DeploymentStep));
    if (steps == NULL)
    {
        return MANIFEST_ALLOC_ERROR;
    }

    size_t parsedCount = 0;

    for (size_t i = 0; i < totalSteps; i++)
    {
        JSON_Object* stepObj = json_array_get_object(stepsArray, i);
        if (stepObj == NULL)
        {
            continue;
        }

        const char* type = json_object_get_string(stepObj, "type");

        /* Skip reference steps for now (child manifest support TBD) */
        if (type != NULL && strcmp(type, "reference") == 0)
        {
            /* TODO: handle child/detached manifests */
            continue;
        }

        ADUC_DeploymentStep* step = &steps[parsedCount];

        /* Generate step ID */
        char stepIdBuf[32];
        snprintf(stepIdBuf, sizeof(stepIdBuf), "step_%zu", i);
        step->stepId = strdup(stepIdBuf);

        /* Handler type */
        const char* handler = json_object_get_string(stepObj, "handler");
        step->handlerType = handler ? strdup(handler) : NULL;

        /* Handler properties -> serialize to JSON string */
        JSON_Value* propsValue = json_object_get_value(stepObj, "handlerProperties");
        if (propsValue != NULL)
        {
            char* serialized = json_serialize_to_string(propsValue);
            step->handlerConfigJson = serialized ? strdup(serialized) : NULL;
            json_free_serialized_string(serialized);

            /* Extract orchestration keys from handlerProperties */
            JSON_Object* propsObj = json_value_get_object(propsValue);
            if (propsObj != NULL)
            {
                /* "id" overrides auto-generated stepId */
                const char* idOverride = json_object_get_string(propsObj, "id");
                if (idOverride != NULL && idOverride[0] != '\0')
                {
                    free((void*)step->stepId);
                    step->stepId = strdup(idOverride);
                }

                /* "requires" → comma-separated list of dependency step IDs */
                const char* requiresCsv = json_object_get_string(propsObj, "requires");
                if (requiresCsv != NULL)
                {
                    size_t cnt = 0;
                    char** arr = split_csv(requiresCsv, &cnt);
                    step->requires = (const char**)arr;
                    step->requiresCount = cnt;
                }

                /* "skipOnFailed" → comma-separated list */
                const char* skipCsv = json_object_get_string(propsObj, "skipOnFailed");
                if (skipCsv != NULL)
                {
                    size_t cnt = 0;
                    char** arr = split_csv(skipCsv, &cnt);
                    step->skipOnFailed = (const char**)arr;
                    step->skipOnFailedCount = cnt;
                }

                /* "runOnFailed" → comma-separated list */
                const char* rofCsv = json_object_get_string(propsObj, "runOnFailed");
                if (rofCsv != NULL)
                {
                    size_t cnt = 0;
                    char** arr = split_csv(rofCsv, &cnt);
                    step->runOnFailed = (const char**)arr;
                    step->runOnFailedCount = cnt;
                }
            }
        }
        else
        {
            step->handlerConfigJson = NULL;
        }

        /* Installed criteria */
        const char* criteria = json_object_get_string(stepObj, "installedCriteria");
        step->installedCriteria = criteria ? strdup(criteria) : NULL;

        /* Component fields (not present in Gen1 manifest, default to NULL) */
        step->componentId = NULL;
        step->componentGroup = NULL;
        step->componentProperties = NULL;

        /* Resolve file references */
        JSON_Array* filesArray = json_object_get_array(stepObj, "files");
        if (filesArray != NULL)
        {
            size_t stepFileCount = json_array_get_count(filesArray);
            /*
             * Build an array of pointers into the global file table.
             * We store as a contiguous ADUC_StepFile* pointing to entries
             * in the global table (the step does not own these).
             */
            const ADUC_StepFile** fileRefs =
                (const ADUC_StepFile**)calloc(stepFileCount, sizeof(const ADUC_StepFile*));
            size_t resolvedCount = 0;

            if (fileRefs != NULL)
            {
                for (size_t f = 0; f < stepFileCount; f++)
                {
                    const char* fid = json_array_get_string(filesArray, f);
                    const ADUC_StepFile* found = find_file_by_id(globalFiles, globalFileCount, fid);
                    if (found != NULL)
                    {
                        fileRefs[resolvedCount++] = found;
                    }
                }
            }

            /* Store as flat pointer to first element (contiguous in global array) */
            if (resolvedCount > 0 && fileRefs != NULL)
            {
                step->files = fileRefs[0];
                step->fileCount = resolvedCount;
            }
            else
            {
                step->files = NULL;
                step->fileCount = 0;
            }
            free(fileRefs);
        }
        else
        {
            step->files = NULL;
            step->fileCount = 0;
        }

        parsedCount++;
    }

    *outSteps = steps;
    *outCount = parsedCount;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_Manifest_Parse(const char* json, size_t jsonLen, ADUC_ParsedManifest** outManifest)
{
    (void)jsonLen; /* Parson uses null-terminated strings */

    if (json == NULL || outManifest == NULL)
    {
        return MANIFEST_INVALID_ERROR;
    }

    *outManifest = NULL;

    JSON_Value* rootValue = json_parse_string(json);
    if (rootValue == NULL)
    {
        return MANIFEST_PARSE_ERROR;
    }

    JSON_Object* rootObj = json_value_get_object(rootValue);
    if (rootObj == NULL)
    {
        json_value_free(rootValue);
        return MANIFEST_PARSE_ERROR;
    }

    ADUC_ParsedManifest* manifest = (ADUC_ParsedManifest*)calloc(1, sizeof(ADUC_ParsedManifest));
    if (manifest == NULL)
    {
        json_value_free(rootValue);
        return MANIFEST_ALLOC_ERROR;
    }

    /* Parse workflowId */
    const char* workflowId = json_object_get_string(rootObj, "workflowId");
    safe_strcpy(manifest->workflowId, sizeof(manifest->workflowId), workflowId);

    /* Parse updateId */
    JSON_Object* updateIdObj = json_object_get_object(rootObj, "updateId");
    if (updateIdObj != NULL)
    {
        safe_strcpy(manifest->updateId.provider, sizeof(manifest->updateId.provider),
                    json_object_get_string(updateIdObj, "provider"));
        safe_strcpy(manifest->updateId.name, sizeof(manifest->updateId.name),
                    json_object_get_string(updateIdObj, "name"));
        safe_strcpy(manifest->updateId.version, sizeof(manifest->updateId.version),
                    json_object_get_string(updateIdObj, "version"));
    }

    /* Parse files */
    JSON_Object* filesObj = json_object_get_object(rootObj, "files");
    if (filesObj != NULL)
    {
        ADUC_Result2 result = parse_files(filesObj, &manifest->files, &manifest->fileCount);
        if (ADUC_RESULT2_IS_FAILURE(result))
        {
            free(manifest);
            json_value_free(rootValue);
            return result;
        }
    }

    /* Parse instructions.steps */
    JSON_Object* instructionsObj = json_object_get_object(rootObj, "instructions");
    if (instructionsObj != NULL)
    {
        JSON_Array* stepsArray = json_object_get_array(instructionsObj, "steps");
        if (stepsArray != NULL)
        {
            ADUC_Result2 result = parse_steps(
                stepsArray, manifest->files, manifest->fileCount,
                &manifest->steps, &manifest->stepCount);
            if (ADUC_RESULT2_IS_FAILURE(result))
            {
                /* Free already-parsed files before returning */
                for (size_t i = 0; i < manifest->fileCount; i++)
                {
                    free((void*)manifest->files[i].id);
                    free((void*)manifest->files[i].name);
                    free((void*)manifest->files[i].sha256);
                }
                free(manifest->files);
                free(manifest);
                json_value_free(rootValue);
                return result;
            }
        }
    }

    json_value_free(rootValue);
    *outManifest = manifest;
    return ADUC_RESULT2_SUCCESS;
}

void ADUC_Manifest_Free(ADUC_ParsedManifest* manifest)
{
    if (manifest == NULL)
    {
        return;
    }

    /* Free steps */
    if (manifest->steps != NULL)
    {
        for (size_t i = 0; i < manifest->stepCount; i++)
        {
            ADUC_DeploymentStep* step = &manifest->steps[i];
            free((void*)step->stepId);
            free((void*)step->handlerType);
            free((void*)step->handlerConfigJson);
            free((void*)step->installedCriteria);
            /* componentId, componentGroup, componentProperties are NULL for Gen1 */
            /* files pointers are into the global table - not owned by the step */

            /* Free orchestration arrays */
            if (step->requires != NULL)
            {
                for (size_t r = 0; r < step->requiresCount; r++)
                {
                    free((void*)step->requires[r]);
                }
                free((void*)step->requires);
            }
            if (step->skipOnFailed != NULL)
            {
                for (size_t r = 0; r < step->skipOnFailedCount; r++)
                {
                    free((void*)step->skipOnFailed[r]);
                }
                free((void*)step->skipOnFailed);
            }
            if (step->runOnFailed != NULL)
            {
                for (size_t r = 0; r < step->runOnFailedCount; r++)
                {
                    free((void*)step->runOnFailed[r]);
                }
                free((void*)step->runOnFailed);
            }
        }
        free(manifest->steps);
    }

    /* Free global file table */
    if (manifest->files != NULL)
    {
        for (size_t i = 0; i < manifest->fileCount; i++)
        {
            free((void*)manifest->files[i].id);
            free((void*)manifest->files[i].name);
            free((void*)manifest->files[i].path);
            free((void*)manifest->files[i].sha256);
        }
        free(manifest->files);
    }

    free(manifest);
}

const ADUC_StepFile* ADUC_Manifest_GetFile(const ADUC_ParsedManifest* manifest, const char* fileId)
{
    if (manifest == NULL || fileId == NULL)
    {
        return NULL;
    }
    return find_file_by_id(manifest->files, manifest->fileCount, fileId);
}
