/**
 * @file microsoft_delta_download_handler_utils_cpp_ut.cpp
 * @brief Non-mock tests for microsoft_delta_download_handler_utils.cpp.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

#include <aduc/microsoft_delta_download_handler_utils.h>

extern const char* AduDiffSharedLibName;

TEST_CASE("ProcessDeltaUpdate fails for non-existent input files")
{
    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate("/tmp/no-src-file", "/tmp/no-delta-file", "/tmp/no-target-file");

    CHECK(result.ResultCode == ADUC_Result_Failure);
}

TEST_CASE("ProcessDeltaUpdate returns processor load error when shared library is missing")
{
    const char* originalLibName = AduDiffSharedLibName;
    AduDiffSharedLibName = "libadudiffapi_missing_for_ut.so";

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate("/tmp/src.bin", "/tmp/delta.bin", "/tmp/target.bin");

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_PROCESSOR_LOAD_LIB);

    AduDiffSharedLibName = originalLibName;
}

TEST_CASE("ProcessDeltaUpdate returns ensure-symbols error when library lacks required exports")
{
    const char* originalLibName = AduDiffSharedLibName;
    AduDiffSharedLibName = "libc.so.6";

    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate("/tmp/src.bin", "/tmp/delta.bin", "/tmp/target.bin");

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_PROCESSOR_ENSURE_SYMBOLS);

    AduDiffSharedLibName = originalLibName;
}
