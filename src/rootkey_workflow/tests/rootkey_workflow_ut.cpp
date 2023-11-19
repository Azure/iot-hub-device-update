/**
 * @file rootkey_workflow_ut.cpp
 * @brief Unit Tests for rootkey_workflow library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/rootkey_workflow.h" // RootKeyWorkflow_UpdateRootKeys
#include <aduc/c_utils.h>
#include <aduc/result.h>
#include <catch2/catch.hpp>
#include <root_key_util.h> // RootKeyUtility_GetReportingErc

using Catch::Matchers::Equals;

TEST_CASE("RootKeyWorkflow_UpdateRootKeys")
{
    SECTION("rootkeyutil reporting erc set on failure")
    {
        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys(nullptr /* workflowId */, nullptr /* workFolder */, nullptr /* rootKeyPkgUrl */);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_INVALIDARG);

        ADUC_Result_t ercForReporting = RootKeyUtility_GetReportingErc();
        CHECK(ercForReporting == result.ExtendedResultCode);
    }
}
