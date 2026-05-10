/**
 * @file content_processor_vtable.h
 * @brief Extension vtable for content processors (decrypt, decompress, delta, etc.)
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_CONTENT_PROCESSOR_VTABLE_H
#define ADUC_CONTENT_PROCESSOR_VTABLE_H

#include "aduc/extension_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Processing mode for content processors.
 */
typedef enum ADUC_ContentProcessMode
{
    ADUC_CONTENT_PROCESS_STREAMING = 0, // Process data in chunks
    ADUC_CONTENT_PROCESS_FILE = 1,      // Process entire file at once
} ADUC_ContentProcessMode;

/**
 * @brief Progress callback for long operations.
 */
typedef void (*ADUC_ContentProgressFn)(void* ctx, uint64_t bytesProcessed, uint64_t bytesTotal);

/**
 * @brief Vtable for content processor extensions.
 */
typedef struct ADUC_ContentProcessorVtable
{
    /** Get supported mode */
    ADUC_ContentProcessMode (*GetMode)(void);

    /** For STREAMING mode: process a chunk, write output to outBuf */
    ADUC_Result2 (*ProcessChunk)(
        const uint8_t* inBuf,
        size_t inLen,
        uint8_t* outBuf,
        size_t outBufLen,
        size_t* outWritten);

    /** For FILE mode: process input file, write to output file */
    ADUC_Result2 (*ProcessFile)(
        const char* inputPath,
        const char* outputPath,
        ADUC_ContentProgressFn progressFn,
        void* progressCtx);

    /** Finalize (flush remaining buffered data for streaming) */
    ADUC_Result2 (*Finalize)(uint8_t* outBuf, size_t outBufLen, size_t* outWritten);

    /** Reset state (for reuse) */
    void (*Reset)(void);

    /** Get output size estimate (0 if unknown) */
    uint64_t (*EstimateOutputSize)(uint64_t inputSize);
} ADUC_ContentProcessorVtable;

#ifdef __cplusplus
}
#endif

#endif // ADUC_CONTENT_PROCESSOR_VTABLE_H
