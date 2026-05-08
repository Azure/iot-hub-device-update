/**
 * @file delta_processor_ut.cpp
 * @brief Unit tests for the delta content processor extension.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern "C"
{
#include "aduc/content_processor_vtable.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_types.h"

    // Import the extension entry point
    const ADUC_ExtensionDescriptor* ADUC_GetExtensionDescriptor(void);
}

// Helper: write bytes to a temp file, return its path.
// Files are created in the build directory (cwd) to avoid /tmp usage.
static std::string write_test_file(const std::string& name, const void* data, size_t len)
{
    std::string path = std::string("test_delta_") + name;
    FILE* f = fopen(path.c_str(), "wb");
    REQUIRE(f != nullptr);
    if (len > 0)
    {
        REQUIRE(fwrite(data, 1, len, f) == len);
    }
    fclose(f);
    return path;
}

static void cleanup_file(const std::string& path)
{
    remove(path.c_str());
}

// =====================================================================
// Extension descriptor tests
// =====================================================================

TEST_CASE("Descriptor has correct fields")
{
    const ADUC_ExtensionDescriptor* desc = ADUC_GetExtensionDescriptor();
    REQUIRE(desc != nullptr);

    CHECK(desc->structVersion == ADUC_EXTENSION_DESCRIPTOR_VERSION);
    CHECK(std::string(desc->id) == "microsoft.adu.processor.delta");
    CHECK(std::string(desc->name) == "delta_processor");
    CHECK(std::string(desc->version) == "2.0.0");
    CHECK(desc->type == ADUC_EXT_TYPE_CONTENT_PROCESSOR);
    CHECK(desc->minHostApiVersion == ADUC_HOST_API_VERSION);
    CHECK(desc->vtable != nullptr);
    CHECK(desc->capabilities != nullptr);
    CHECK(std::string(desc->capabilities[0]) == "microsoft/delta:2");
    CHECK(desc->capabilities[1] == nullptr);
    CHECK(desc->Initialize != nullptr);
    CHECK(desc->Uninitialize != nullptr);
}

TEST_CASE("Initialize and Uninitialize succeed")
{
    const ADUC_ExtensionDescriptor* desc = ADUC_GetExtensionDescriptor();
    REQUIRE(desc != nullptr);

    ADUC_Result2 r = desc->Initialize(nullptr);
    CHECK(ADUC_RESULT2_IS_SUCCESS(r));

    // Should not crash
    desc->Uninitialize();
}

// =====================================================================
// Vtable tests
// =====================================================================

static const ADUC_ContentProcessorVtable* get_vtable()
{
    const ADUC_ExtensionDescriptor* desc = ADUC_GetExtensionDescriptor();
    REQUIRE(desc != nullptr);
    return static_cast<const ADUC_ContentProcessorVtable*>(desc->vtable);
}

TEST_CASE("GetMode returns FILE mode")
{
    auto* vt = get_vtable();
    CHECK(vt->GetMode() == ADUC_CONTENT_PROCESS_FILE);
}

TEST_CASE("ProcessChunk returns error (file-only processor)")
{
    auto* vt = get_vtable();
    uint8_t in[] = {1, 2, 3};
    uint8_t out[16];
    size_t written = 0;

    ADUC_Result2 r = vt->ProcessChunk(in, sizeof(in), out, sizeof(out), &written);
    CHECK(ADUC_RESULT2_IS_FAILURE(r));
    CHECK(written == 0);
}

TEST_CASE("ProcessFile succeeds with copy-mode (no delta header)")
{
    auto* vt = get_vtable();

    const char payload[] = "Hello, this is the full target file content for testing.";
    std::string srcPath = write_test_file("copy_input.bin", payload, sizeof(payload) - 1);
    std::string dstPath = "test_delta_copy_output.bin";

    ADUC_Result2 r = vt->ProcessFile(srcPath.c_str(), dstPath.c_str(), nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_SUCCESS(r));

    // Verify output matches input
    FILE* f = fopen(dstPath.c_str(), "rb");
    REQUIRE(f != nullptr);
    char buf[256] = {};
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    CHECK(n == sizeof(payload) - 1);
    CHECK(memcmp(buf, payload, n) == 0);

    cleanup_file(srcPath);
    cleanup_file(dstPath);
}

TEST_CASE("ProcessFile fails with empty input (DELTA_FORMAT_UNKNOWN)")
{
    auto* vt = get_vtable();

    std::string srcPath = write_test_file("empty_input.bin", nullptr, 0);
    std::string dstPath = "test_delta_empty_output.bin";

    ADUC_Result2 r = vt->ProcessFile(srcPath.c_str(), dstPath.c_str(), nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(r));

    cleanup_file(srcPath);
    cleanup_file(dstPath);
}

TEST_CASE("ProcessFile fails with BSDIFF40 header (stub not implemented)")
{
    auto* vt = get_vtable();

    // Construct a fake BSDIFF40 file
    uint8_t data[64] = {};
    memcpy(data, "BSDIFF40", 8);

    std::string srcPath = write_test_file("bsdiff_input.bin", data, sizeof(data));
    std::string dstPath = "test_delta_bsdiff_output.bin";

    ADUC_Result2 r = vt->ProcessFile(srcPath.c_str(), dstPath.c_str(), nullptr, nullptr);
    // Stub returns failure for real delta formats
    CHECK(ADUC_RESULT2_IS_FAILURE(r));

    cleanup_file(srcPath);
    cleanup_file(dstPath);
}

TEST_CASE("ProcessFile detects ZSTD magic and falls back to copy mode")
{
    auto* vt = get_vtable();

    // ZSTD magic followed by some payload
    uint8_t data[32];
    data[0] = 0x28;
    data[1] = 0xb5;
    data[2] = 0x2f;
    data[3] = 0xfd;
    memset(data + 4, 0xAA, sizeof(data) - 4);

    std::string srcPath = write_test_file("zstd_input.bin", data, sizeof(data));
    std::string dstPath = "test_delta_zstd_output.bin";

    // ZSTD detection hits copy-mode fallback (decompression is stubbed)
    ADUC_Result2 r = vt->ProcessFile(srcPath.c_str(), dstPath.c_str(), nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_SUCCESS(r));

    cleanup_file(srcPath);
    cleanup_file(dstPath);
}

TEST_CASE("ProcessFile fails with null arguments")
{
    auto* vt = get_vtable();

    ADUC_Result2 r1 = vt->ProcessFile(nullptr, "out.bin", nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(r1));

    ADUC_Result2 r2 = vt->ProcessFile("in.bin", nullptr, nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(r2));
}

TEST_CASE("ProcessFile fails with nonexistent input file")
{
    auto* vt = get_vtable();

    ADUC_Result2 r = vt->ProcessFile(
        "test_delta_nonexistent_file_xyz.bin", "test_delta_out.bin", nullptr, nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(r));
}

TEST_CASE("ProcessFile reports progress via callback")
{
    auto* vt = get_vtable();

    // Write a test file larger than one buffer read
    std::vector<uint8_t> payload(1024, 0x42);
    std::string srcPath = write_test_file("progress_input.bin", payload.data(), payload.size());
    std::string dstPath = "test_delta_progress_output.bin";

    struct ProgressState
    {
        int callCount;
        uint64_t lastProcessed;
        uint64_t lastTotal;
    };
    ProgressState state = {0, 0, 0};

    auto progressFn = [](void* ctx, uint64_t processed, uint64_t total) {
        auto* s = static_cast<ProgressState*>(ctx);
        s->callCount++;
        s->lastProcessed = processed;
        s->lastTotal = total;
    };

    ADUC_Result2 r = vt->ProcessFile(srcPath.c_str(), dstPath.c_str(), progressFn, &state);
    CHECK(ADUC_RESULT2_IS_SUCCESS(r));
    CHECK(state.callCount > 0);
    CHECK(state.lastProcessed == 1024);
    CHECK(state.lastTotal == 1024);

    cleanup_file(srcPath);
    cleanup_file(dstPath);
}

TEST_CASE("Finalize and Reset do not crash")
{
    auto* vt = get_vtable();

    uint8_t buf[16];
    size_t written = 99;
    ADUC_Result2 r = vt->Finalize(buf, sizeof(buf), &written);
    CHECK(ADUC_RESULT2_IS_SUCCESS(r));
    CHECK(written == 0);

    // Reset should not crash
    vt->Reset();
}

TEST_CASE("EstimateOutputSize returns 0")
{
    auto* vt = get_vtable();
    CHECK(vt->EstimateOutputSize(12345) == 0);
}
