/**
 * @file delta_processor.c
 * @brief Content processor extension for microsoft/delta:2.
 *
 * Applies binary delta patches to produce updated files. Supports:
 * - BSDIFF40 format detection (stub — logs intent, returns unsupported)
 * - ADUDELTA2 custom format detection (stub)
 * - ZSTD-compressed payload detection (magic 0x28b52ffd)
 * - Copy-mode fallback: when the input has no recognized delta header,
 *   it is treated as the full target file (for functional testing without
 *   actual delta generation).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/content_processor_vtable.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_types.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DELTA_LOG_TAG "DeltaProcessor"

// Extension-local logging (avoids linking log_writer.c which has PIC issues)
static void delta_log(const char* level, const char* fmt, ...)
{
    time_t now = time(NULL);
    struct tm tm_buf;
    struct tm* tm = localtime_r(&now, &tm_buf);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", tm);
    fprintf(stderr, "[%s][%s][%s] ", ts, level, DELTA_LOG_TAG);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

#define LOG_INFO(...)  delta_log("INFO", __VA_ARGS__)
#define LOG_ERROR(...) delta_log("ERROR", __VA_ARGS__)

// Magic bytes for format detection
static const uint8_t BSDIFF40_MAGIC[8] = { 'B','S','D','I','F','F','4','0' };
static const uint8_t ADUDELTA2_MAGIC[8] = { 'A','D','U','D','E','L','T','2' };
static const uint8_t ZSTD_MAGIC[4] = { 0x28, 0xb5, 0x2f, 0xfd };

typedef enum DeltaFormat
{
    DELTA_FORMAT_UNKNOWN = 0,
    DELTA_FORMAT_BSDIFF40,
    DELTA_FORMAT_ADUDELTA2,
    DELTA_FORMAT_COPY,  // No delta header — treat as full file
} DeltaFormat;

/**
 * @brief Detect the delta format from the first bytes of the file.
 */
static DeltaFormat detect_format(const char* filePath, bool* outIsZstdCompressed)
{
    *outIsZstdCompressed = false;

    FILE* f = fopen(filePath, "rb");
    if (!f)
    {
        return DELTA_FORMAT_UNKNOWN;
    }

    uint8_t header[8];
    size_t bytesRead = fread(header, 1, sizeof(header), f);
    fclose(f);

    if (bytesRead < 8)
    {
        // File too small for any delta header — treat as copy mode
        return (bytesRead > 0) ? DELTA_FORMAT_COPY : DELTA_FORMAT_UNKNOWN;
    }

    // Check ZSTD compression first (the delta payload might be compressed)
    if (memcmp(header, ZSTD_MAGIC, 4) == 0)
    {
        *outIsZstdCompressed = true;
        LOG_INFO("ZSTD-compressed payload detected; decompression required (stub)");
        // After decompression we'd re-check the inner header; for now treat as copy
        return DELTA_FORMAT_COPY;
    }

    if (memcmp(header, BSDIFF40_MAGIC, 8) == 0)
    {
        return DELTA_FORMAT_BSDIFF40;
    }

    if (memcmp(header, ADUDELTA2_MAGIC, 8) == 0)
    {
        return DELTA_FORMAT_ADUDELTA2;
    }

    // No recognized header — copy mode
    return DELTA_FORMAT_COPY;
}

/**
 * @brief Copy a file byte-for-byte (copy-mode fallback).
 */
static ADUC_Result2 copy_file(
    const char* srcPath,
    const char* dstPath,
    ADUC_ContentProgressFn progressFn,
    void* progressCtx)
{
    FILE* src = fopen(srcPath, "rb");
    if (!src)
    {
        LOG_ERROR("Cannot open source: %s", srcPath);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 0x0010);
    }

    // Get file size for progress reporting
    fseek(src, 0, SEEK_END);
    uint64_t totalSize = (uint64_t)ftell(src);
    fseek(src, 0, SEEK_SET);

    FILE* dst = fopen(dstPath, "wb");
    if (!dst)
    {
        fclose(src);
        LOG_ERROR("Cannot open destination: %s", dstPath);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 0x0011);
    }

    uint8_t buf[65536];
    uint64_t bytesWritten = 0;
    size_t n;

    while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
    {
        if (fwrite(buf, 1, n, dst) != n)
        {
            fclose(src);
            fclose(dst);
            LOG_ERROR("Write error at offset %lu", (unsigned long)bytesWritten);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 0x0012);
        }
        bytesWritten += n;

        if (progressFn)
        {
            progressFn(progressCtx, bytesWritten, totalSize);
        }
    }

    fclose(src);
    fclose(dst);

    LOG_INFO("Copy-mode: copied %lu bytes from %s to %s",
        (unsigned long)bytesWritten, srcPath, dstPath);

    return ADUC_RESULT2_SUCCESS;
}

// ─── Vtable Implementation ──────────────────────────────────────────────────

static ADUC_ContentProcessMode Delta_GetMode(void)
{
    return ADUC_CONTENT_PROCESS_FILE;
}

static ADUC_Result2 Delta_ProcessChunk(
    const uint8_t* inBuf,
    size_t inLen,
    uint8_t* outBuf,
    size_t outBufLen,
    size_t* outWritten)
{
    // Delta processor only supports FILE mode
    (void)inBuf; (void)inLen; (void)outBuf; (void)outBufLen;
    if (outWritten) *outWritten = 0;
    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_STATE, 0x0001);
}

static ADUC_Result2 Delta_ProcessFile(
    const char* inputPath,
    const char* outputPath,
    ADUC_ContentProgressFn progressFn,
    void* progressCtx)
{
    if (!inputPath || !outputPath)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_STATE, 0x0002);
    }

    LOG_INFO("Processing delta: input=%s output=%s", inputPath, outputPath);

    bool isZstdCompressed = false;
    DeltaFormat format = detect_format(inputPath, &isZstdCompressed);

    switch (format)
    {
        case DELTA_FORMAT_BSDIFF40:
            LOG_INFO("BSDIFF40 delta detected — bspatch application (stub, not yet implemented)");
            // In production this would call bspatch with a source base file.
            // For now return an error indicating the real implementation is pending.
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_STATE, 0x0003);

        case DELTA_FORMAT_ADUDELTA2:
            LOG_INFO("ADUDELTA2 delta detected — proprietary delta application (stub)");
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_STATE, 0x0003);

        case DELTA_FORMAT_COPY:
            LOG_INFO("No delta header — using copy-mode fallback");
            return copy_file(inputPath, outputPath, progressFn, progressCtx);

        case DELTA_FORMAT_UNKNOWN:
        default:
            LOG_ERROR("Unrecognized or empty input file: %s", inputPath);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 0x0020);
    }
}

static ADUC_Result2 Delta_Finalize(uint8_t* outBuf, size_t outBufLen, size_t* outWritten)
{
    // No streaming state to flush
    (void)outBuf; (void)outBufLen;
    if (outWritten) *outWritten = 0;
    return ADUC_RESULT2_SUCCESS;
}

static void Delta_Reset(void)
{
    // No persistent state
}

static uint64_t Delta_EstimateOutputSize(uint64_t inputSize)
{
    // Cannot estimate without parsing delta header
    (void)inputSize;
    return 0;
}

// ─── Extension Descriptor ───────────────────────────────────────────────────

static ADUC_Result2 Delta_Initialize(const ADUC_ExtensionContext* ctx)
{
    (void)ctx;
    LOG_INFO("Delta processor v2.0.0 initialized");
    return ADUC_RESULT2_SUCCESS;
}

static void Delta_Uninitialize(void)
{
    LOG_INFO("Delta processor uninitialized");
}

static const ADUC_ContentProcessorVtable s_vtable = {
    .GetMode = Delta_GetMode,
    .ProcessChunk = Delta_ProcessChunk,
    .ProcessFile = Delta_ProcessFile,
    .Finalize = Delta_Finalize,
    .Reset = Delta_Reset,
    .EstimateOutputSize = Delta_EstimateOutputSize,
};

static const char* s_capabilities[] = { "microsoft/delta:2", NULL };

static const ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = ADUC_EXTENSION_DESCRIPTOR_VERSION,
    .id = "microsoft.adu.processor.delta",
    .name = "delta_processor",
    .version = "2.0.0",
    .type = ADUC_EXT_TYPE_CONTENT_PROCESSOR,
    .minHostApiVersion = ADUC_HOST_API_VERSION,
    .Initialize = Delta_Initialize,
    .Uninitialize = Delta_Uninitialize,
    .vtable = &s_vtable,
    .capabilities = s_capabilities,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
