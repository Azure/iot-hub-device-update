#include <catch2/catch_all.hpp>

#include <aduc/agent_workflow.h>
#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/workflow_utils.h>
#include <aducpal/stdlib.h>

#include <cstdlib>
#include <memory>
#include <string>

#include "aduc/steps_handler.hpp"

extern "C" {
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);
}

namespace {

void SetTestConfig()
{
    std::string testDataFolder = ADUC_TEST_DATA_FOLDER;
    std::string configFolder = testDataFolder + "/script_handler_test_config";
    ADUCPAL_setenv("ADUC_CONFIG_FOLDER_ENV", configFolder.c_str(), 1);
}

} // namespace

TEST_CASE("GetContractInfo returns expected values", "[steps_handler][non_mock]")
{
    ADUC_ExtensionContractInfo contractInfo {};

    ADUC_Result result = GetContractInfo(&contractInfo);

    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    CHECK(contractInfo.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(contractInfo.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

TEST_CASE("CreateUpdateContentHandlerExtension returns handler", "[steps_handler][non_mock]")
{
    SetTestConfig();

    std::unique_ptr<ContentHandler> handler(CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG));
    REQUIRE(handler != nullptr);
}

TEST_CASE("Steps handler factory create returns handler", "[steps_handler][non_mock]")
{
    auto handler = StepsHandlerImpl::CreateContentHandler();
    REQUIRE(handler != nullptr);
}
