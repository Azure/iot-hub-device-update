/**
 * @file microsoft_delta_download_handler_utils_cpp_ut.cpp
 * @brief Non-mock tests for microsoft_delta_download_handler_utils.cpp.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

#include <aduc/microsoft_delta_download_handler_utils.h>

TEST_CASE("ProcessDeltaUpdate fails for non-existent input files")
{
    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate("/tmp/no-src-file", "/tmp/no-delta-file", "/tmp/no-target-file");

    CHECK(result.ResultCode == ADUC_Result_Failure);
}
