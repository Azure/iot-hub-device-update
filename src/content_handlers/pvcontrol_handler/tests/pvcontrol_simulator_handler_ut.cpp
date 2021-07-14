#include "aduc/adu_core_exports.h"
#include "aduc/pvcontrol_simulator_handler.hpp"
#include <catch2/catch.hpp>

const std::string workFolder{ "/tmp" };
const std::string& logFolder{ "/log" };
const std::string& filename{ "test.swu" };

TEST_CASE("Simulator Handler content handler prepare test")
{
    std::unique_ptr<ContentHandler> handler =
        PVControlSimulatorHandlerImpl::CreateContentHandler(workFolder, logFolder, filename);

    CHECK(handler);

    ADUC_PrepareInfo info = {};
    char updateType[21] = "pantacor/pvcontrol:1";
    char updateTypeName[19] = "pantacor/pvcontrol";
    info.updateType = updateType;
    info.updateTypeName = updateTypeName;

    SECTION("Prepare Success")
    {
        info.fileCount = 1;
        info.updateTypeVersion = 1;
        ADUC_Result result = handler->Prepare(&info);
        CHECK(result.ResultCode == ADUC_PrepareResult_Success);
    }

    SECTION("Prepare Fail with Wrong File Count")
    {
        info.fileCount = 2;
        info.updateTypeVersion = 1;
        ADUC_Result result = handler->Prepare(&info);
        CHECK(result.ResultCode == ADUC_PrepareResult_Failure);
        CHECK(result.ExtendedResultCode == ADUC_ERC_PVCONTROL_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT);
    }

    SECTION("PVCONTROL handler prepare wrong version")
    {
        info.fileCount = 1;
        info.updateTypeVersion = 2;
        ADUC_Result result = handler->Prepare(&info);
        CHECK(result.ResultCode == ADUC_PrepareResult_Failure);
        CHECK(result.ExtendedResultCode == ADUC_ERC_PVCONTROL_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_VERSION);
    }
}

TEST_CASE("Simulator Handler content handler download test")
{
    std::unique_ptr<ContentHandler> handler =
        PVControlSimulatorHandlerImpl::CreateContentHandler(workFolder, logFolder, filename);

    CHECK(handler);

    ADUC_Result result = handler->Download();
    CHECK(result.ResultCode == ADUC_DownloadResult_Success);
}

TEST_CASE("Simulator Handler content handler install test")
{
    std::unique_ptr<ContentHandler> handler =
        PVControlSimulatorHandlerImpl::CreateContentHandler(workFolder, logFolder, filename);
    CHECK(handler);

    ADUC_Result result = handler->Install();
    CHECK(result.ResultCode == ADUC_InstallResult_Success);
}

TEST_CASE("Simulator Handler content handler apply test")
{
    std::unique_ptr<ContentHandler> handler =
        PVControlSimulatorHandlerImpl::CreateContentHandler(workFolder, logFolder, filename);

    CHECK(handler);

    ADUC_Result result = handler->Apply();
    CHECK(result.ResultCode == ADUC_ApplyResult_Success);
}

TEST_CASE("Simulator Handler content handler cancel test")
{
    std::unique_ptr<ContentHandler> handler =
        PVControlSimulatorHandlerImpl::CreateContentHandler(workFolder, logFolder, filename);

    CHECK(handler);

    ADUC_Result result = handler->Cancel();
    CHECK(result.ResultCode == ADUC_CancelResult_Success);
}

TEST_CASE("Simulator Handler content handler isInstalled test")
{
    std::unique_ptr<ContentHandler> handler =
        PVControlSimulatorHandlerImpl::CreateContentHandler(workFolder, logFolder, filename);

    std::string fakeInstalledCriteria = "asdfg";
    CHECK(handler);

    ADUC_Result result = handler->Apply();
    CHECK(result.ResultCode == ADUC_InstallResult_Success);

    result = handler->IsInstalled(fakeInstalledCriteria);

    CHECK(result.ResultCode == ADUC_IsInstalledResult_Installed);
}
