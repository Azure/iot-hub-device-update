/**
 * @file content_pipeline.h
 * @brief Composable pipeline that chains content processors.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_CONTENT_PIPELINE_H
#define ADUC_CONTENT_PIPELINE_H

#include "aduc/content_processor_vtable.h"
#include "aduc/extension_types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ADUC_ContentPipeline* ADUC_ContentPipelineHandle;

/**
 * @brief Create an empty content pipeline.
 */
ADUC_Result2 ADUC_ContentPipeline_Create(ADUC_ContentPipelineHandle* outHandle);

/**
 * @brief Add a processor stage (processors execute in order added).
 */
ADUC_Result2 ADUC_ContentPipeline_AddStage(
    ADUC_ContentPipelineHandle handle,
    const char* name,
    const ADUC_ContentProcessorVtable* processor);

/**
 * @brief Process a file through all stages.
 */
ADUC_Result2 ADUC_ContentPipeline_ProcessFile(
    ADUC_ContentPipelineHandle handle,
    const char* inputPath,
    const char* outputPath,
    ADUC_ContentProgressFn progressFn,
    void* progressCtx);

/**
 * @brief Get stage count.
 */
size_t ADUC_ContentPipeline_GetStageCount(ADUC_ContentPipelineHandle handle);

/**
 * @brief Reset all stages.
 */
void ADUC_ContentPipeline_Reset(ADUC_ContentPipelineHandle handle);

/**
 * @brief Destroy a content pipeline and free resources.
 */
void ADUC_ContentPipeline_Destroy(ADUC_ContentPipelineHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ADUC_CONTENT_PIPELINE_H
