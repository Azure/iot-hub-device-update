/**
 * @file content_pipeline_ut.cpp
 * @brief Unit tests for the content pipeline.
 */

#include <catch2/catch_all.hpp>

#include <cstdio>
#include <cstring>
#include <string>

extern "C"
{
#include "aduc/content_pipeline.h"
#include "aduc/content_processor_vtable.h"
}

static const char* TEST_INPUT_FILE = "test_pipeline_input.bin";
static const char* TEST_OUTPUT_FILE = "test_pipeline_output.bin";

class TempPipelineFiles
{
public:
    TempPipelineFiles(const char* content = nullptr, size_t len = 0)
    {
        if (content != nullptr && len > 0)
        {
            FILE* f = fopen(TEST_INPUT_FILE, "wb");
            if (f != nullptr)
            {
                fwrite(content, 1, len, f);
                fclose(f);
            }
        }
    }
    ~TempPipelineFiles()
    {
        remove(TEST_INPUT_FILE);
        remove(TEST_OUTPUT_FILE);
    }
};

// Passthrough processor vtable for testing
static ADUC_ContentProcessMode passthrough_GetMode(void)
{
    return ADUC_CONTENT_PROCESS_FILE;
}

static ADUC_Result2 passthrough_ProcessFile(
    const char* inputPath,
    const char* outputPath,
    ADUC_ContentProgressFn progressFn,
    void* progressCtx)
{
    FILE* fin = fopen(inputPath, "rb");
    if (fin == nullptr)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    FILE* fout = fopen(outputPath, "wb");
    if (fout == nullptr)
    {
        fclose(fin);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 2);
    }

    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fin)) > 0)
    {
        fwrite(buf, 1, n, fout);
    }

    fclose(fin);
    fclose(fout);

    (void)progressFn;
    (void)progressCtx;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 passthrough_ProcessChunk(
    const uint8_t* inBuf, size_t inLen,
    uint8_t* outBuf, size_t outBufLen, size_t* outWritten)
{
    (void)inBuf; (void)inLen; (void)outBuf; (void)outBufLen; (void)outWritten;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 passthrough_Finalize(uint8_t* outBuf, size_t outBufLen, size_t* outWritten)
{
    (void)outBuf; (void)outBufLen; (void)outWritten;
    return ADUC_RESULT2_SUCCESS;
}

static void passthrough_Reset(void) {}

static uint64_t passthrough_EstimateOutputSize(uint64_t inputSize)
{
    return inputSize;
}

static const ADUC_ContentProcessorVtable s_passthroughVtable = {
    passthrough_GetMode,
    passthrough_ProcessChunk,
    passthrough_ProcessFile,
    passthrough_Finalize,
    passthrough_Reset,
    passthrough_EstimateOutputSize,
};

// ─── Content Pipeline Tests ──────────────────────────────────────────────────

TEST_CASE("ContentPipeline: Create pipeline succeeds", "[content_pipeline]")
{
    ADUC_ContentPipelineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_ContentPipeline_Create(&handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(handle != nullptr);

    ADUC_ContentPipeline_Destroy(handle);
}

TEST_CASE("ContentPipeline: Empty pipeline ProcessFile copies input to output", "[content_pipeline]")
{
    const char* content = "test content data for pipeline";
    TempPipelineFiles tmp(content, strlen(content));

    ADUC_ContentPipelineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_ContentPipeline_Create(&handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    result = ADUC_ContentPipeline_ProcessFile(handle, TEST_INPUT_FILE, TEST_OUTPUT_FILE, nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));

    // Verify output matches input
    FILE* f = fopen(TEST_OUTPUT_FILE, "rb");
    REQUIRE(f != nullptr);
    char buf[256] = {};
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    CHECK(n == strlen(content));
    CHECK(memcmp(buf, content, n) == 0);

    ADUC_ContentPipeline_Destroy(handle);
}

TEST_CASE("ContentPipeline: Add stages and verify count", "[content_pipeline]")
{
    ADUC_ContentPipelineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_ContentPipeline_Create(&handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(ADUC_ContentPipeline_GetStageCount(handle) == 0);

    result = ADUC_ContentPipeline_AddStage(handle, "passthrough1", &s_passthroughVtable);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_ContentPipeline_GetStageCount(handle) == 1);

    result = ADUC_ContentPipeline_AddStage(handle, "passthrough2", &s_passthroughVtable);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(ADUC_ContentPipeline_GetStageCount(handle) == 2);

    ADUC_ContentPipeline_Destroy(handle);
}

TEST_CASE("ContentPipeline: Pipeline with passthrough processor", "[content_pipeline]")
{
    const char* content = "hello pipeline world";
    TempPipelineFiles tmp(content, strlen(content));

    ADUC_ContentPipelineHandle handle = nullptr;
    ADUC_Result2 result = ADUC_ContentPipeline_Create(&handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    result = ADUC_ContentPipeline_AddStage(handle, "passthrough", &s_passthroughVtable);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    result = ADUC_ContentPipeline_ProcessFile(handle, TEST_INPUT_FILE, TEST_OUTPUT_FILE, nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));

    // Verify output matches input (passthrough)
    FILE* f = fopen(TEST_OUTPUT_FILE, "rb");
    REQUIRE(f != nullptr);
    char buf[256] = {};
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    CHECK(n == strlen(content));
    CHECK(memcmp(buf, content, n) == 0);

    ADUC_ContentPipeline_Destroy(handle);
}

TEST_CASE("ContentPipeline: Destroy NULL does not crash", "[content_pipeline]")
{
    ADUC_ContentPipeline_Destroy(nullptr); // Should not crash
}
