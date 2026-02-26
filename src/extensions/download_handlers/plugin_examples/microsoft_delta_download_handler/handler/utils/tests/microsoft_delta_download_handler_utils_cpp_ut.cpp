/**
 * @file microsoft_delta_download_handler_utils_cpp_ut.cpp
 * @brief Unit Tests for microsoft_delta_download_handler_utils.cpp — ProcessDeltaUpdate.
 *
 * Uses a mock aduc::SharedLib class and mock diff API functions so that
 * ProcessDeltaUpdate can be tested without a real libadudiffapi.so.
 *
 * Tests:
 *  - SharedLib load failure (dlopen)
 *  - EnsureSymbols failure (missing symbol)
 *  - create_session returns NULL
 *  - diff apply succeeds
 *  - diff apply fails with error details
 *  - diff apply fails with no errors
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

#include "mock_diff_api.h"
#include <aduc/microsoft_delta_download_handler_utils.h>
#include <aduc/result.h>

// =====================================================================
// ProcessDeltaUpdate tests
// =====================================================================

TEST_CASE("ProcessDeltaUpdate: SharedLib load failure -> failure with ADUC_ERC_DDH_PROCESSOR_LOAD_LIB")
{
    ResetDiffApiMocks();
    g_mockDiffApi.sharedLibShouldThrow = true;

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate("/src.swu", "/delta.dat", "/target.swu");

    CHECK(result.ResultCode == ADUC_Result_Failure);
    // The exception is caught before ERC is overwritten by ENSURE_SYMBOLS
    // so the ERC stays as ADUC_ERC_DDH_PROCESSOR_LOAD_LIB
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_PROCESSOR_LOAD_LIB);
}

TEST_CASE("ProcessDeltaUpdate: EnsureSymbols failure -> failure with ADUC_ERC_DDH_PROCESSOR_ENSURE_SYMBOLS")
{
    ResetDiffApiMocks();
    g_mockDiffApi.ensureSymbolsShouldThrow = true;

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate("/src.swu", "/delta.dat", "/target.swu");

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_PROCESSOR_ENSURE_SYMBOLS);
}

TEST_CASE("ProcessDeltaUpdate: create_session returns NULL -> failure with ADUC_ERC_DDH_PROCESSOR_CREATE_SESSION")
{
    ResetDiffApiMocks();
    g_mockDiffApi.createSessionResult = nullptr;

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate("/src.swu", "/delta.dat", "/target.swu");

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_PROCESSOR_CREATE_SESSION);
    CHECK(g_mockDiffApi.createSessionCallCount == 1);
    CHECK(g_mockDiffApi.closeSessionCallCount == 0); // session was NULL
}

TEST_CASE("ProcessDeltaUpdate: diff apply succeeds -> success")
{
    ResetDiffApiMocks();
    g_mockDiffApi.applyResult = 0;

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate("/src.swu", "/delta.dat", "/target.swu");

    CHECK(result.ResultCode == ADUC_Result_Success);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(g_mockDiffApi.createSessionCallCount == 1);
    CHECK(g_mockDiffApi.applyCallCount == 1);
    CHECK(g_mockDiffApi.closeSessionCallCount == 1);
}

TEST_CASE("ProcessDeltaUpdate: diff apply fails with errors -> failure with error details")
{
    ResetDiffApiMocks();
    g_mockDiffApi.applyResult = 42; // non-zero = error
    g_mockDiffApi.errorCount = 1;
    g_mockDiffApi.errorCode = 99;
    g_mockDiffApi.errorText = "bad diff data";

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate("/src.swu", "/delta.dat", "/target.swu");

    CHECK(result.ResultCode == ADUC_Result_Failure);
    // The last error code overwrites the ERC
    CHECK(result.ExtendedResultCode == MAKE_DELTA_PROCESSOR_EXTENDEDRESULTCODE(99));
    CHECK(g_mockDiffApi.applyCallCount == 1);
    CHECK(g_mockDiffApi.closeSessionCallCount == 1);
}

TEST_CASE("ProcessDeltaUpdate: diff apply fails with no errors -> failure with overall error code")
{
    ResetDiffApiMocks();
    g_mockDiffApi.applyResult = 7;
    g_mockDiffApi.errorCount = 0; // no individual errors

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate("/src.swu", "/delta.dat", "/target.swu");

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == MAKE_DELTA_PROCESSOR_EXTENDEDRESULTCODE(7));
    CHECK(g_mockDiffApi.closeSessionCallCount == 1);
}

TEST_CASE("ProcessDeltaUpdate: diff apply fails with multiple errors -> last error code used")
{
    ResetDiffApiMocks();
    g_mockDiffApi.applyResult = 1;
    g_mockDiffApi.errorCount = 3;
    g_mockDiffApi.errorCode = 55;
    g_mockDiffApi.errorText = "multiple errors";

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate("/src.swu", "/delta.dat", "/target.swu");

    CHECK(result.ResultCode == ADUC_Result_Failure);
    // All 3 errors have the same mock error code; last one wins
    CHECK(result.ExtendedResultCode == MAKE_DELTA_PROCESSOR_EXTENDEDRESULTCODE(55));
}
