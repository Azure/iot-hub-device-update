/**
 * @file content_handler_ut.cpp
 * @brief Unit Tests for ContentHandler interface (SetContractInfo, GetContractInfo).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/content_handler.hpp>
#include <aduc/contract_utils.h>
#include <aduc/result.h>
#include <catch2/catch_all.hpp>

// Concrete test subclass to test the non-virtual methods
class TestContentHandler : public ContentHandler
{
public:
    TestContentHandler() = default;
    ~TestContentHandler() override = default;

    ADUC_Result Download(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }

    ADUC_Result Backup(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }

    ADUC_Result Install(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }

    ADUC_Result Apply(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }

    ADUC_Result Restore(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }

    ADUC_Result Cancel(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }

    ADUC_Result IsInstalled(const tagADUC_WorkflowData* /*workflowData*/) override
    {
        return ADUC_Result{ 1, 0 };
    }
};

TEST_CASE("ContentHandler default contractInfo is zero-initialized")
{
    TestContentHandler handler;
    ADUC_ExtensionContractInfo info = handler.GetContractInfo();
    CHECK(info.majorVer == 0);
    CHECK(info.minorVer == 0);
}

TEST_CASE("ContentHandler SetContractInfo stores and GetContractInfo retrieves")
{
    TestContentHandler handler;

    ADUC_ExtensionContractInfo info{ 2, 5 };
    handler.SetContractInfo(info);

    ADUC_ExtensionContractInfo retrieved = handler.GetContractInfo();
    CHECK(retrieved.majorVer == 2);
    CHECK(retrieved.minorVer == 5);
}

TEST_CASE("ContentHandler SetContractInfo can overwrite previous value")
{
    TestContentHandler handler;

    ADUC_ExtensionContractInfo info1{ 1, 0 };
    handler.SetContractInfo(info1);

    ADUC_ExtensionContractInfo info2{ 3, 7 };
    handler.SetContractInfo(info2);

    ADUC_ExtensionContractInfo retrieved = handler.GetContractInfo();
    CHECK(retrieved.majorVer == 3);
    CHECK(retrieved.minorVer == 7);
}

TEST_CASE("ContentHandler SetContractInfo with v1 contract")
{
    TestContentHandler handler;

    ADUC_ExtensionContractInfo info{ ADUC_V1_CONTRACT_MAJOR_VER, ADUC_V1_CONTRACT_MINOR_VER };
    handler.SetContractInfo(info);

    ADUC_ExtensionContractInfo retrieved = handler.GetContractInfo();
    CHECK(retrieved.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(retrieved.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

TEST_CASE("ContentHandler virtual method implementations return success")
{
    TestContentHandler handler;

    ADUC_Result result = handler.Download(nullptr);
    CHECK(result.ResultCode == 1);

    result = handler.Backup(nullptr);
    CHECK(result.ResultCode == 1);

    result = handler.Install(nullptr);
    CHECK(result.ResultCode == 1);

    result = handler.Apply(nullptr);
    CHECK(result.ResultCode == 1);

    result = handler.Restore(nullptr);
    CHECK(result.ResultCode == 1);

    result = handler.Cancel(nullptr);
    CHECK(result.ResultCode == 1);

    result = handler.IsInstalled(nullptr);
    CHECK(result.ResultCode == 1);
}

TEST_CASE("ContentHandler can be used through base class pointer")
{
    ContentHandler* handler = new TestContentHandler();

    ADUC_ExtensionContractInfo info{ 4, 2 };
    handler->SetContractInfo(info);

    ADUC_ExtensionContractInfo retrieved = handler->GetContractInfo();
    CHECK(retrieved.majorVer == 4);
    CHECK(retrieved.minorVer == 2);

    delete handler;
}
