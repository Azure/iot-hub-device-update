/**
 * @file manifest_parser_ut.cpp
 * @brief Unit tests for the deployment manifest parser.
 */

#include <catch2/catch_all.hpp>

#include <cstring>
#include <string>

extern "C"
{
#include "aduc/manifest_parser.h"
#include "aduc/extension_types.h"
}

static const char* const kValidManifestJson = R"({
    "workflowId": "wf-001",
    "updateId": {
        "provider": "Contoso",
        "name": "Toaster",
        "version": "2.0.0"
    },
    "files": {
        "f1": {
            "fileName": "firmware.bin",
            "sizeInBytes": 1048576,
            "hashes": {
                "sha256": "abc123def456"
            }
        },
        "f2": {
            "fileName": "config.json",
            "sizeInBytes": 256,
            "hashes": {
                "sha256": "789xyz"
            }
        }
    },
    "instructions": {
        "steps": [
            {
                "handler": "microsoft/swupdate:2",
                "handlerProperties": {
                    "installedCriteria": "2.0.0"
                },
                "installedCriteria": "2.0.0",
                "files": ["f1"]
            },
            {
                "handler": "microsoft/script:1",
                "installedCriteria": "1.0.0",
                "files": ["f2"]
            }
        ]
    }
})";

TEST_CASE("ManifestParser: Parse valid manifest", "[manifest_parser]")
{
    ADUC_ParsedManifest* manifest = nullptr;
    ADUC_Result2 result = ADUC_Manifest_Parse(kValidManifestJson, strlen(kValidManifestJson), &manifest);

    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(manifest != nullptr);

    ADUC_Manifest_Free(manifest);
}

TEST_CASE("ManifestParser: Verify parsed fields", "[manifest_parser]")
{
    ADUC_ParsedManifest* manifest = nullptr;
    ADUC_Result2 result = ADUC_Manifest_Parse(kValidManifestJson, strlen(kValidManifestJson), &manifest);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(manifest != nullptr);

    // Verify workflowId
    CHECK(std::string(manifest->workflowId) == "wf-001");

    // Verify updateId
    CHECK(std::string(manifest->updateId.provider) == "Contoso");
    CHECK(std::string(manifest->updateId.name) == "Toaster");
    CHECK(std::string(manifest->updateId.version) == "2.0.0");

    // Verify files
    REQUIRE(manifest->fileCount == 2);
    REQUIRE(manifest->files != nullptr);

    // Verify steps
    REQUIRE(manifest->stepCount == 2);
    REQUIRE(manifest->steps != nullptr);

    // First step
    CHECK(std::string(manifest->steps[0].stepId) == "step_0");
    CHECK(std::string(manifest->steps[0].handlerType) == "microsoft/swupdate:2");
    CHECK(std::string(manifest->steps[0].installedCriteria) == "2.0.0");
    CHECK(manifest->steps[0].fileCount == 1);

    // Second step
    CHECK(std::string(manifest->steps[1].stepId) == "step_1");
    CHECK(std::string(manifest->steps[1].handlerType) == "microsoft/script:1");

    ADUC_Manifest_Free(manifest);
}

TEST_CASE("ManifestParser: Verify file details", "[manifest_parser]")
{
    ADUC_ParsedManifest* manifest = nullptr;
    ADUC_Result2 result = ADUC_Manifest_Parse(kValidManifestJson, strlen(kValidManifestJson), &manifest);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(manifest != nullptr);
    REQUIRE(manifest->fileCount == 2);

    // Files may be in any order since they come from a JSON object
    bool foundF1 = false;
    bool foundF2 = false;
    for (size_t i = 0; i < manifest->fileCount; i++)
    {
        if (manifest->files[i].id != nullptr && std::string(manifest->files[i].id) == "f1")
        {
            foundF1 = true;
            CHECK(std::string(manifest->files[i].name) == "firmware.bin");
            CHECK(manifest->files[i].size == 1048576);
            CHECK(std::string(manifest->files[i].sha256) == "abc123def456");
        }
        else if (manifest->files[i].id != nullptr && std::string(manifest->files[i].id) == "f2")
        {
            foundF2 = true;
            CHECK(std::string(manifest->files[i].name) == "config.json");
            CHECK(manifest->files[i].size == 256);
        }
    }
    CHECK(foundF1);
    CHECK(foundF2);

    ADUC_Manifest_Free(manifest);
}

TEST_CASE("ManifestParser: GetFile by ID", "[manifest_parser]")
{
    ADUC_ParsedManifest* manifest = nullptr;
    ADUC_Result2 result = ADUC_Manifest_Parse(kValidManifestJson, strlen(kValidManifestJson), &manifest);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    const ADUC_StepFile* file = ADUC_Manifest_GetFile(manifest, "f1");
    REQUIRE(file != nullptr);
    CHECK(std::string(file->name) == "firmware.bin");

    // Non-existent file ID
    CHECK(ADUC_Manifest_GetFile(manifest, "f99") == nullptr);

    ADUC_Manifest_Free(manifest);
}

TEST_CASE("ManifestParser: Parse invalid JSON returns error", "[manifest_parser]")
{
    const char* badJson = "{ this is not valid json !!!";
    ADUC_ParsedManifest* manifest = nullptr;

    ADUC_Result2 result = ADUC_Manifest_Parse(badJson, strlen(badJson), &manifest);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
    CHECK(manifest == nullptr);
}

TEST_CASE("ManifestParser: Parse NULL input returns error", "[manifest_parser]")
{
    ADUC_ParsedManifest* manifest = nullptr;

    ADUC_Result2 result = ADUC_Manifest_Parse(nullptr, 0, &manifest);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("ManifestParser: Parse NULL outManifest returns error", "[manifest_parser]")
{
    const char* json = "{}";
    ADUC_Result2 result = ADUC_Manifest_Parse(json, strlen(json), nullptr);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("ManifestParser: Free NULL manifest is safe", "[manifest_parser]")
{
    ADUC_Manifest_Free(nullptr); // Should not crash
}

TEST_CASE("ManifestParser: GetFile on NULL manifest returns NULL", "[manifest_parser]")
{
    CHECK(ADUC_Manifest_GetFile(nullptr, "f1") == nullptr);
}

TEST_CASE("ManifestParser: GetFile with NULL fileId returns NULL", "[manifest_parser]")
{
    ADUC_ParsedManifest* manifest = nullptr;
    ADUC_Result2 result = ADUC_Manifest_Parse(kValidManifestJson, strlen(kValidManifestJson), &manifest);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(ADUC_Manifest_GetFile(manifest, nullptr) == nullptr);

    ADUC_Manifest_Free(manifest);
}

TEST_CASE("ManifestParser: Parse minimal valid JSON (empty manifest)", "[manifest_parser]")
{
    const char* minimalJson = R"({"workflowId": "wf-minimal"})";
    ADUC_ParsedManifest* manifest = nullptr;

    ADUC_Result2 result = ADUC_Manifest_Parse(minimalJson, strlen(minimalJson), &manifest);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(manifest != nullptr);

    CHECK(std::string(manifest->workflowId) == "wf-minimal");
    CHECK(manifest->stepCount == 0);
    CHECK(manifest->fileCount == 0);

    ADUC_Manifest_Free(manifest);
}

TEST_CASE("ManifestParser: Reference steps are skipped", "[manifest_parser]")
{
    const char* json = R"({
        "workflowId": "wf-ref",
        "instructions": {
            "steps": [
                {
                    "type": "reference",
                    "detachedManifestFileId": "dm1"
                },
                {
                    "handler": "microsoft/apt:1",
                    "installedCriteria": "1.0"
                }
            ]
        }
    })";

    ADUC_ParsedManifest* manifest = nullptr;
    ADUC_Result2 result = ADUC_Manifest_Parse(json, strlen(json), &manifest);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    REQUIRE(manifest != nullptr);

    // Only the non-reference step should be parsed
    CHECK(manifest->stepCount == 1);
    CHECK(std::string(manifest->steps[0].handlerType) == "microsoft/apt:1");

    ADUC_Manifest_Free(manifest);
}
