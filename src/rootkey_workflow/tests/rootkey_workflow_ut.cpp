/**
 * @file rootkey_workflow_ut.cpp
 * @brief Unit Tests for rootkey_workflow library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/rootkey_workflow.h" // RootKeyWorkflow_UpdateRootKeys
#include "aduc/types/adu_core.h"
#include <aduc/c_utils.h>
#include <aduc/result.h>
#include <catch2/catch.hpp>
#include <root_key_util.h> // RootKeyUtility_GetReportingErc
#include <rootkey_store.hpp> // RootKeyUtility_GetReportingErc

using Catch::Matchers::Equals;

class MockRootKeyStore : public IRootKeyStoreInternal
{
public:
    MockRootKeyStore() : m_pkg(nullptr) {}
    bool SetConfig(RootKeyStoreConfigProperty propertyName, const char* propertyValue) noexcept override { return true; }
    const std::string* GetConfig(RootKeyStoreConfigProperty propertyName) const noexcept override { return new std::string{""}; }
    bool SetRootKeyPackage(const ADUC_RootKeyPackage* package) noexcept override
    {
        m_pkg = const_cast<ADUC_RootKeyPackage*>(package);
        return true;
    }

    bool GetRootKeyPackage(ADUC_RootKeyPackage* outPackage) noexcept override
    {
        *outPackage = *m_pkg;
        m_pkg = nullptr;
        return true;
    }
    bool Load() noexcept override { return true; }
    ADUC_Result Persist() noexcept override { return { ADUC_Result_Success, 0 }; }

private:
    ADUC_RootKeyPackage* m_pkg;
};

TEST_CASE("RootKeyWorkflow_UpdateRootKeys")
{
    SECTION("rootkeyutil reporting erc set on failure")
    {
        MockRootKeyStore store;
        RootKeyUtilContext context =
        {
            &store, /* rootKeyStoreHandle */
            0, /* rootKeyExtendedResult */
        };

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys(&context, nullptr /* workflowId */, nullptr /* rootKeyPkgUrl */);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_INVALIDARG);

        ADUC_Result_t ercForReporting = RootKeyUtility_GetReportingErc(&context);
        CHECK(ercForReporting == result.ExtendedResultCode);
    }
}
