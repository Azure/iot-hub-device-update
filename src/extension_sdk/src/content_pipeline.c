/**
 * @file content_pipeline.c
 * @brief Composable pipeline that chains content processors.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef _WIN32
#define _GNU_SOURCE
#endif

#include "aduc/content_pipeline.h"
#include "aduc/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#define MAX_STAGES 8
#define PIPELINE_NAME_MAX 64
#define PIPE_ERR_INVALID_ARG   ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 0x0010)
#define PIPE_ERR_ALLOC         ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 0x0010)
#define PIPE_ERR_TOO_MANY      ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 0x0011)
#define PIPE_ERR_NO_STAGES     ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_STATE, 0x0010)
#define PIPE_ERR_STAGE_FAILED  ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 0x0011)
#define PIPE_ERR_TEMP_FILE     ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 0x0012)

typedef struct PipelineStage
{
    char name[PIPELINE_NAME_MAX];
    const ADUC_ContentProcessorVtable* processor;
} PipelineStage;

struct ADUC_ContentPipeline
{
    PipelineStage stages[MAX_STAGES];
    size_t stageCount;
};

ADUC_Result2 ADUC_ContentPipeline_Create(ADUC_ContentPipelineHandle* outHandle)
{
    if (outHandle == NULL)
    {
        return PIPE_ERR_INVALID_ARG;
    }

    struct ADUC_ContentPipeline* pipeline = (struct ADUC_ContentPipeline*)calloc(1, sizeof(struct ADUC_ContentPipeline));
    if (pipeline == NULL)
    {
        return PIPE_ERR_ALLOC;
    }

    *outHandle = pipeline;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_ContentPipeline_AddStage(
    ADUC_ContentPipelineHandle handle,
    const char* name,
    const ADUC_ContentProcessorVtable* processor)
{
    if (handle == NULL || name == NULL || processor == NULL)
    {
        return PIPE_ERR_INVALID_ARG;
    }

    if (handle->stageCount >= MAX_STAGES)
    {
        return PIPE_ERR_TOO_MANY;
    }

    PipelineStage* stage = &handle->stages[handle->stageCount];
    strncpy(stage->name, name, PIPELINE_NAME_MAX - 1);
    stage->name[PIPELINE_NAME_MAX - 1] = '\0';
    stage->processor = processor;
    handle->stageCount++;

    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_ContentPipeline_ProcessFile(
    ADUC_ContentPipelineHandle handle,
    const char* inputPath,
    const char* outputPath,
    ADUC_ContentProgressFn progressFn,
    void* progressCtx)
{
    if (handle == NULL || inputPath == NULL || outputPath == NULL)
    {
        return PIPE_ERR_INVALID_ARG;
    }

    if (handle->stageCount == 0)
    {
        // Empty pipeline acts as passthrough: copy input to output
        FILE* fin = fopen(inputPath, "rb");
        if (fin == NULL)
        {
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 0x0013);
        }
        FILE* fout = fopen(outputPath, "wb");
        if (fout == NULL)
        {
            fclose(fin);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 0x0014);
        }
        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), fin)) > 0)
        {
            if (fwrite(buf, 1, n, fout) != n)
            {
                fclose(fin);
                fclose(fout);
                return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 0x0015);
            }
        }
        fclose(fin);
        fclose(fout);
        return ADUC_RESULT2_SUCCESS;
    }

    // For a single stage, process directly from input to output
    if (handle->stageCount == 1)
    {
        const ADUC_ContentProcessorVtable* proc = handle->stages[0].processor;
        return proc->ProcessFile(inputPath, outputPath, progressFn, progressCtx);
    }

    // Multiple stages: chain through temp files
    const char* currentInput = inputPath;
    char tempPaths[MAX_STAGES][256];
    size_t tempCount = 0;

    for (size_t i = 0; i < handle->stageCount; i++)
    {
        const ADUC_ContentProcessorVtable* proc = handle->stages[i].processor;
        const char* stageOutput;

        if (i == handle->stageCount - 1)
        {
            // Last stage writes to final output
            stageOutput = outputPath;
        }
        else
        {
            // Create temp file for intermediate output
            snprintf(tempPaths[tempCount], sizeof(tempPaths[tempCount]), ".%caduc_pipe_XXXXXX", ADU_PATH_SEP);
            int fd = adu_mkstemp(tempPaths[tempCount]);
            if (fd < 0)
            {
                // Cleanup previous temps
                for (size_t t = 0; t < tempCount; t++)
                {
                    unlink(tempPaths[t]);
                }
                return PIPE_ERR_TEMP_FILE;
            }
            close(fd);
            stageOutput = tempPaths[tempCount];
            tempCount++;
        }

        // Only pass progress callback to last stage
        ADUC_ContentProgressFn stageProg = (i == handle->stageCount - 1) ? progressFn : NULL;
        void* stageCtx = (i == handle->stageCount - 1) ? progressCtx : NULL;

        ADUC_Result2 result = proc->ProcessFile(currentInput, stageOutput, stageProg, stageCtx);
        if (ADUC_RESULT2_IS_FAILURE(result))
        {
            for (size_t t = 0; t < tempCount; t++)
            {
                unlink(tempPaths[t]);
            }
            return result;
        }

        currentInput = stageOutput;
    }

    // Cleanup temp files
    for (size_t t = 0; t < tempCount; t++)
    {
        unlink(tempPaths[t]);
    }

    return ADUC_RESULT2_SUCCESS;
}

size_t ADUC_ContentPipeline_GetStageCount(ADUC_ContentPipelineHandle handle)
{
    if (handle == NULL)
    {
        return 0;
    }
    return handle->stageCount;
}

void ADUC_ContentPipeline_Reset(ADUC_ContentPipelineHandle handle)
{
    if (handle == NULL)
    {
        return;
    }

    for (size_t i = 0; i < handle->stageCount; i++)
    {
        if (handle->stages[i].processor->Reset != NULL)
        {
            handle->stages[i].processor->Reset();
        }
    }
}

void ADUC_ContentPipeline_Destroy(ADUC_ContentPipelineHandle handle)
{
    free(handle);
}
