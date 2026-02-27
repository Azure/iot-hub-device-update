/**
 * @file handler_create_mock_ut.cpp
 * @brief Tests for handler_create.cpp exception paths.
 *
 * Tests CreateUpdateContentHandlerExtension and GetContractInfo,
 * exercising the catch blocks in CreateUpdateContentHandlerExtension.
 */

#include <catch2/catch_all.hpp>

#include <aduc/content_handler.hpp>
#include <aduc/contract_utils.h>
#include <aduc/result.h>

// The functions under test (from handler_create.cpp)
extern "C" {
ContentHandler* CreateUpdateContentHandlerExtension(int logLevel);
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);
}

// Mock behavior control (defined in mock_handler_create_deps.cpp)
enum class MockCreateBehavior
{
    ReturnValid,
    ThrowStdException,
    ThrowNonStd
};

extern void mock_handler_create_set_behavior(MockCreateBehavior behavior);

// =====================================================================
// GetContractInfo tests
// =====================================================================

TEST_CASE("handler_create: GetContractInfo returns success with correct version")
{
    ADUC_ExtensionContractInfo info{};
    ADUC_Result result = GetContractInfo(&info);
    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(info.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(info.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

// =====================================================================
// CreateUpdateContentHandlerExtension tests
// =====================================================================

TEST_CASE("handler_create: CreateUpdateContentHandlerExtension happy path returns non-null")
{
    mock_handler_create_set_behavior(MockCreateBehavior::ReturnValid);
    ContentHandler* handler = CreateUpdateContentHandlerExtension(0);
    REQUIRE(handler != nullptr);
    delete handler;
}

TEST_CASE("handler_create: CreateUpdateContentHandlerExtension catches std::exception and returns nullptr")
{
    mock_handler_create_set_behavior(MockCreateBehavior::ThrowStdException);
    ContentHandler* handler = CreateUpdateContentHandlerExtension(0);
    CHECK(handler == nullptr);
}

TEST_CASE("handler_create: CreateUpdateContentHandlerExtension catches non-std exception and returns nullptr")
{
    mock_handler_create_set_behavior(MockCreateBehavior::ThrowNonStd);
    ContentHandler* handler = CreateUpdateContentHandlerExtension(0);
    CHECK(handler == nullptr);
}
