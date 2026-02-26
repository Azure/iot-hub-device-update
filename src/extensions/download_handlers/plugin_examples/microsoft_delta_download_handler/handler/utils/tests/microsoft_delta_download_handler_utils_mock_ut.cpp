/**
 * @file microsoft_delta_download_handler_utils_mock_ut.cpp
 * @brief Mock-based Unit Tests for microsoft_delta_download_handler_utils.c
 *
 * Tests the internal functions with link-time mocks for external dependencies:
 *   - GetSourceUpdateProperties: valid properties, STRING_construct failure path
 *   - LookupSourceUpdateCachePath: get-properties fail, get-update-id fail, cache miss, cache hit, lookup fail
 *   - DownloadDeltaUpdate: delegates to ExtensionManager_Download
 *   - GetDeltaUpdateDownloadSandboxPath: null workFolder, STRING_new/STRING_sprintf, success
 *   - ProcessRelatedFile: full success, download failure, sandbox path failure, process delta failure
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

extern "C"
{
#include "mock_utils_deps.h"
#include <aduc/microsoft_delta_download_handler_utils.h>
}

#include <azure_c_shared_utility/strings.h>
#include <cstring>
#include <string>

// -----------------------------------------------------------------------
// Helpers — build an ADUC_RelatedFile with source hash properties
// -----------------------------------------------------------------------

static ADUC_Property s_props[2];

/**
 * @brief Sets up s_props with the two required properties and
 *        populates the relatedFile to point at them.
 *        Caller must NOT free the Name/Value strings (they are literals cast to char*).
 */
static void SetupRelatedFileWithProperties(
    ADUC_RelatedFile& rf, const char* hash = "abc123", const char* alg = "sha256")
{
    memset(&rf, 0, sizeof(rf));

    s_props[0].Name = const_cast<char*>("microsoft.sourceFileHash");
    s_props[0].Value = const_cast<char*>(hash);
    s_props[1].Name = const_cast<char*>("microsoft.sourceFileHashAlgorithm");
    s_props[1].Value = const_cast<char*>(alg);

    rf.Properties = s_props;
    rf.PropertiesCount = 2;
    rf.FileName = const_cast<char*>("delta.dat");
    rf.FileId = const_cast<char*>("file-id-1");
    rf.DownloadUri = const_cast<char*>("http://example.com/delta.dat");
    rf.SizeInBytes = 1024;
}

// =====================================================================
// Mock function-pointers for ProcessRelatedFile
// =====================================================================

static ADUC_Result s_processDeltaResult;
static int s_processDeltaCallCount;
static ADUC_Result s_downloadDeltaResult;
static int s_downloadDeltaCallCount;

static ADUC_Result MockProcessDelta(
    const char* /* sourceUpdateFilePath */,
    const char* /* deltaUpdateFilePath */,
    const char* /* targetUpdateFilePath */)
{
    s_processDeltaCallCount++;
    return s_processDeltaResult;
}

static ADUC_Result MockDownloadDelta(
    const ADUC_WorkflowHandle /* workflowHandle */, const ADUC_RelatedFile* /* relatedFile */)
{
    s_downloadDeltaCallCount++;
    return s_downloadDeltaResult;
}

static void ResetCallbackMocks()
{
    memset(&s_processDeltaResult, 0, sizeof(s_processDeltaResult));
    s_processDeltaCallCount = 0;
    memset(&s_downloadDeltaResult, 0, sizeof(s_downloadDeltaResult));
    s_downloadDeltaCallCount = 0;
}

// =====================================================================
// GetSourceUpdateProperties
// =====================================================================

TEST_CASE("GetSourceUpdateProperties: valid properties -> success")
{
    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    STRING_HANDLE outHash = NULL;
    STRING_HANDLE outAlg = NULL;

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(&rf, &outHash, &outAlg);

    CHECK(result.ResultCode == ADUC_Result_Success);
    REQUIRE(outHash != NULL);
    REQUIRE(outAlg != NULL);
    CHECK(std::string(STRING_c_str(outHash)) == "abc123");
    CHECK(std::string(STRING_c_str(outAlg)) == "sha256");

    STRING_delete(outHash);
    STRING_delete(outAlg);
}

TEST_CASE("GetSourceUpdateProperties: missing hash property -> failure")
{
    ADUC_RelatedFile rf;
    memset(&rf, 0, sizeof(rf));

    // Only algorithm, no hash
    ADUC_Property props[1];
    props[0].Name = const_cast<char*>("microsoft.sourceFileHashAlgorithm");
    props[0].Value = const_cast<char*>("sha256");
    rf.Properties = props;
    rf.PropertiesCount = 1;

    STRING_HANDLE outHash = NULL;
    STRING_HANDLE outAlg = NULL;

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(&rf, &outHash, &outAlg);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_RELATEDFILE_BAD_OR_MISSING_HASH_PROPERTIES);
}

TEST_CASE("GetSourceUpdateProperties: missing algorithm property -> failure")
{
    ADUC_RelatedFile rf;
    memset(&rf, 0, sizeof(rf));

    ADUC_Property props[1];
    props[0].Name = const_cast<char*>("microsoft.sourceFileHash");
    props[0].Value = const_cast<char*>("abc123");
    rf.Properties = props;
    rf.PropertiesCount = 1;

    STRING_HANDLE outHash = NULL;
    STRING_HANDLE outAlg = NULL;

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(&rf, &outHash, &outAlg);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_RELATEDFILE_BAD_OR_MISSING_HASH_PROPERTIES);
}

TEST_CASE("GetSourceUpdateProperties: empty hash value -> failure")
{
    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf, "", "sha256");

    STRING_HANDLE outHash = NULL;
    STRING_HANDLE outAlg = NULL;

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(&rf, &outHash, &outAlg);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_RELATEDFILE_BAD_OR_MISSING_HASH_PROPERTIES);
}

// =====================================================================
// LookupSourceUpdateCachePath
// =====================================================================

TEST_CASE("LookupSourceUpdateCachePath: GetSourceUpdateProperties fails -> failure")
{
    ResetUtilsMocks();

    ADUC_RelatedFile rf;
    memset(&rf, 0, sizeof(rf));
    rf.PropertiesCount = 0;
    rf.Properties = NULL;

    int dummyHandle = 0;
    STRING_HANDLE outPath = NULL;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_LookupSourceUpdateCachePath(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), &rf, NULL, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(outPath == NULL);
}

TEST_CASE("LookupSourceUpdateCachePath: workflow_get_expected_update_id fails -> failure")
{
    ResetUtilsMocks();

    g_mockUtilsDeps.getUpdateIdResult.ResultCode = ADUC_Result_Failure;
    g_mockUtilsDeps.getUpdateIdResult.ExtendedResultCode = 0xDEAD;

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    STRING_HANDLE outPath = NULL;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_LookupSourceUpdateCachePath(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), &rf, NULL, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(outPath == NULL);
    CHECK(g_mockUtilsDeps.getUpdateIdCallCount == 1);
}

TEST_CASE("LookupSourceUpdateCachePath: cache lookup fails -> failure")
{
    ResetUtilsMocks();

    g_mockUtilsDeps.getUpdateIdResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.updateIdProvider = const_cast<char*>("contoso");
    g_mockUtilsDeps.updateIdName = const_cast<char*>("device");
    g_mockUtilsDeps.updateIdVersion = const_cast<char*>("1.0");

    g_mockUtilsDeps.lookupResult.ResultCode = ADUC_Result_Failure;
    g_mockUtilsDeps.lookupResult.ExtendedResultCode = 0xAAAA;

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    STRING_HANDLE outPath = NULL;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_LookupSourceUpdateCachePath(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), &rf, NULL, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(outPath == NULL);
    CHECK(g_mockUtilsDeps.lookupCallCount == 1);
}

TEST_CASE("LookupSourceUpdateCachePath: cache miss -> ADUC_Result_Success_Cache_Miss")
{
    ResetUtilsMocks();

    g_mockUtilsDeps.getUpdateIdResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.updateIdProvider = const_cast<char*>("contoso");

    g_mockUtilsDeps.lookupResult.ResultCode = ADUC_Result_Success_Cache_Miss;

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    STRING_HANDLE outPath = NULL;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_LookupSourceUpdateCachePath(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), &rf, NULL, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Success_Cache_Miss);
    CHECK(outPath == NULL);
}

TEST_CASE("LookupSourceUpdateCachePath: cache hit -> success with path")
{
    ResetUtilsMocks();

    g_mockUtilsDeps.getUpdateIdResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.updateIdProvider = const_cast<char*>("contoso");

    g_mockUtilsDeps.lookupResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.lookupOutPath = STRING_construct("/cache/source.swu");

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    STRING_HANDLE outPath = NULL;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_LookupSourceUpdateCachePath(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), &rf, NULL, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Success);
    REQUIRE(outPath != NULL);

    // Note: LookupSourceUpdateCachePath calls free() on the returned STRING_HANDLE
    // if it transfers ownership. The outPath is the same handle set via mock.
}

// =====================================================================
// DownloadDeltaUpdate
// =====================================================================

TEST_CASE("DownloadDeltaUpdate: delegates to ExtensionManager_Download and returns success")
{
    ResetUtilsMocks();

    g_mockUtilsDeps.downloadResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.downloadResult.ExtendedResultCode = 0;

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_DownloadDeltaUpdate(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), &rf);

    CHECK(result.ResultCode == ADUC_Result_Success);
    CHECK(g_mockUtilsDeps.downloadCallCount == 1);
}

TEST_CASE("DownloadDeltaUpdate: download failure is propagated")
{
    ResetUtilsMocks();

    g_mockUtilsDeps.downloadResult.ResultCode = ADUC_Result_Failure;
    g_mockUtilsDeps.downloadResult.ExtendedResultCode = 0xBBBB;

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_DownloadDeltaUpdate(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), &rf);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == 0xBBBB);
}

// =====================================================================
// GetDeltaUpdateDownloadSandboxPath
// =====================================================================

TEST_CASE("GetDeltaUpdateDownloadSandboxPath: null workFolder -> ADUC_ERC_NOMEM")
{
    ResetUtilsMocks();

    g_mockUtilsDeps.workFolder = NULL;

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    STRING_HANDLE outPath = NULL;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_GetDeltaUpdateDownloadSandboxPath(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), &rf, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_NOMEM);
    CHECK(outPath == NULL);
}

TEST_CASE("GetDeltaUpdateDownloadSandboxPath: success constructs path")
{
    ResetUtilsMocks();

    g_mockUtilsDeps.workFolder = const_cast<char*>("/tmp/sandbox");

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    STRING_HANDLE outPath = NULL;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_GetDeltaUpdateDownloadSandboxPath(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), &rf, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Success);
    REQUIRE(outPath != NULL);
    CHECK(std::string(STRING_c_str(outPath)) == "/tmp/sandbox/delta.dat");

    STRING_delete(outPath);
}

// =====================================================================
// ProcessRelatedFile (full paths with mocks)
// =====================================================================

TEST_CASE("ProcessRelatedFile: lookup fails -> propagated failure")
{
    ResetUtilsMocks();
    ResetCallbackMocks();

    // Make GetSourceUpdateProperties fail (no properties)
    ADUC_RelatedFile rf;
    memset(&rf, 0, sizeof(rf));
    rf.PropertiesCount = 0;
    rf.Properties = NULL;

    int dummyHandle = 0;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle),
        &rf,
        "/target.swu",
        NULL,
        MockProcessDelta,
        MockDownloadDelta);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(s_downloadDeltaCallCount == 0);
    CHECK(s_processDeltaCallCount == 0);
}

TEST_CASE("ProcessRelatedFile: cache miss -> returns cache miss")
{
    ResetUtilsMocks();
    ResetCallbackMocks();

    g_mockUtilsDeps.getUpdateIdResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.updateIdProvider = const_cast<char*>("contoso");
    g_mockUtilsDeps.lookupResult.ResultCode = ADUC_Result_Success_Cache_Miss;

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle),
        &rf,
        "/target.swu",
        NULL,
        MockProcessDelta,
        MockDownloadDelta);

    CHECK(result.ResultCode == ADUC_Result_Success_Cache_Miss);
    CHECK(s_downloadDeltaCallCount == 0);
}

TEST_CASE("ProcessRelatedFile: download delta fails -> failure")
{
    ResetUtilsMocks();
    ResetCallbackMocks();

    // Lookup: cache hit
    g_mockUtilsDeps.getUpdateIdResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.updateIdProvider = const_cast<char*>("contoso");
    g_mockUtilsDeps.lookupResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.lookupOutPath = STRING_construct("/cache/src.swu");

    // Download fails
    s_downloadDeltaResult.ResultCode = ADUC_Result_Failure;
    s_downloadDeltaResult.ExtendedResultCode = 0x1111;

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle),
        &rf,
        "/target.swu",
        NULL,
        MockProcessDelta,
        MockDownloadDelta);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(s_downloadDeltaCallCount == 1);
    CHECK(s_processDeltaCallCount == 0);
}

TEST_CASE("ProcessRelatedFile: sandbox path fails -> failure")
{
    ResetUtilsMocks();
    ResetCallbackMocks();

    // Lookup: cache hit
    g_mockUtilsDeps.getUpdateIdResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.updateIdProvider = const_cast<char*>("contoso");
    g_mockUtilsDeps.lookupResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.lookupOutPath = STRING_construct("/cache/src.swu");

    // Download succeeds
    s_downloadDeltaResult.ResultCode = ADUC_Result_Success;

    // workFolder is NULL -> sandbox path fails
    g_mockUtilsDeps.workFolder = NULL;

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle),
        &rf,
        "/target.swu",
        NULL,
        MockProcessDelta,
        MockDownloadDelta);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(s_downloadDeltaCallCount == 1);
    CHECK(s_processDeltaCallCount == 0);
}

TEST_CASE("ProcessRelatedFile: process delta fails -> failure")
{
    ResetUtilsMocks();
    ResetCallbackMocks();

    // Lookup: cache hit
    g_mockUtilsDeps.getUpdateIdResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.updateIdProvider = const_cast<char*>("contoso");
    g_mockUtilsDeps.lookupResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.lookupOutPath = STRING_construct("/cache/src.swu");

    // Download and sandbox path succeed
    s_downloadDeltaResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.workFolder = const_cast<char*>("/tmp/sandbox");

    // Process delta fails
    s_processDeltaResult.ResultCode = ADUC_Result_Failure;
    s_processDeltaResult.ExtendedResultCode = 0x2222;

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle),
        &rf,
        "/target.swu",
        NULL,
        MockProcessDelta,
        MockDownloadDelta);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(s_downloadDeltaCallCount == 1);
    CHECK(s_processDeltaCallCount == 1);
}

TEST_CASE("ProcessRelatedFile: full success path")
{
    ResetUtilsMocks();
    ResetCallbackMocks();

    // Lookup: cache hit
    g_mockUtilsDeps.getUpdateIdResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.updateIdProvider = const_cast<char*>("contoso");
    g_mockUtilsDeps.lookupResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.lookupOutPath = STRING_construct("/cache/src.swu");

    // Download, sandbox, and process all succeed
    s_downloadDeltaResult.ResultCode = ADUC_Result_Success;
    g_mockUtilsDeps.workFolder = const_cast<char*>("/tmp/sandbox");
    s_processDeltaResult.ResultCode = ADUC_Result_Success;

    ADUC_RelatedFile rf;
    SetupRelatedFileWithProperties(rf);

    int dummyHandle = 0;
    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle),
        &rf,
        "/target.swu",
        NULL,
        MockProcessDelta,
        MockDownloadDelta);

    CHECK(result.ResultCode == ADUC_Result_Success);
    CHECK(s_downloadDeltaCallCount == 1);
    CHECK(s_processDeltaCallCount == 1);
}
