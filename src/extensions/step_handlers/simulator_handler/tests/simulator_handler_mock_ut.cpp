/**
 * @file simulator_handler_mock_ut.cpp
 * @brief Mock-based unit tests for simulator_handler.cpp targeting 85%+ coverage.
 */
#include "mock_simulator_handler_deps.h"

#include <aduc/simulator_handler.hpp>
#include <aduc/result.h>
#include <aduc/types/workflow.h>

#include <catch2/catch_all.hpp>
using Catch::Matchers::Equals;

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

/* =====================================================================
 * Declarations for functions we test
 * ===================================================================== */
EXTERN_C_BEGIN
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);
const char* _GetTemporaryPathName();
char* _StringFormat(const char* fmt, ...);
EXTERN_C_END
// GetSimulatorDataFilePath is declared with C++ linkage in the header
char* GetSimulatorDataFilePath();

/* Helper: build a minimal tagADUC_WorkflowData */
static tagADUC_WorkflowData make_workflow_data()
{
    tagADUC_WorkflowData wd{};
    static int dummy_handle = 1;
    wd.WorkflowHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy_handle);
    return wd;
}

/* Helper: write a simulator data file and return path (must free) */
static char* write_sim_data(const char* json)
{
    char* path = GetSimulatorDataFilePath();
    REQUIRE(path != nullptr);
    std::ofstream f(path, std::ios::trunc | std::ios::binary);
    f.write(json, strlen(json));
    REQUIRE(!f.bad());
    return path;
}

/* =====================================================================
 * CreateUpdateContentHandlerExtension
 * ===================================================================== */

TEST_CASE("Simulator: CreateUpdateContentHandlerExtension returns handler", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    ContentHandler* h = CreateUpdateContentHandlerExtension(ADUC_LOG_INFO);
    REQUIRE(h != nullptr);
    delete h;
}

/* =====================================================================
 * GetContractInfo
 * ===================================================================== */

TEST_CASE("Simulator: GetContractInfo fills version", "[simulator_handler]")
{
    ADUC_ExtensionContractInfo info{};
    ADUC_Result r = GetContractInfo(&info);
    CHECK(r.ResultCode == ADUC_GeneralResult_Success);
    CHECK(info.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(info.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}

/* =====================================================================
 * _GetTemporaryPathName
 * ===================================================================== */

TEST_CASE("Simulator: _GetTemporaryPathName returns a path", "[simulator_handler]")
{
    const char* path = _GetTemporaryPathName();
    REQUIRE(path != nullptr);
    CHECK(strlen(path) > 0);
}

/* =====================================================================
 * _StringFormat
 * ===================================================================== */

TEST_CASE("Simulator: _StringFormat - null fmt returns null", "[simulator_handler]")
{
    char* s = _StringFormat(nullptr);
    CHECK(s == nullptr);
}

TEST_CASE("Simulator: _StringFormat - simple format", "[simulator_handler]")
{
    char* s = _StringFormat("hello %s %d", "world", 42);
    REQUIRE(s != nullptr);
    CHECK(strcmp(s, "hello world 42") == 0);
    free(s);
}

TEST_CASE("Simulator: _StringFormat - long string returns null", "[simulator_handler]")
{
    // Create a format string that would produce output > 512 chars
    std::string longStr(600, 'A');
    char* s = _StringFormat("%s", longStr.c_str());
    CHECK(s == nullptr);
}

/* =====================================================================
 * GetSimulatorDataFilePath
 * ===================================================================== */

TEST_CASE("Simulator: GetSimulatorDataFilePath returns valid path", "[simulator_handler]")
{
    char* path = GetSimulatorDataFilePath();
    REQUIRE(path != nullptr);
    CHECK(strlen(path) > 0);
    // Should contain the data file name
    CHECK(strstr(path, "du-simulator-data.json") != nullptr);
    free(path);
}

/* =====================================================================
 * CreateContentHandler
 * ===================================================================== */

TEST_CASE("Simulator: CreateContentHandler returns non-null", "[simulator_handler]")
{
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    REQUIRE(h != nullptr);
    delete h;
}

/* =====================================================================
 * Download tests
 * ===================================================================== */

TEST_CASE("Simulator: Download - no data file, default success", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    // Remove data file if it exists
    char* path = GetSimulatorDataFilePath();
    remove(path);
    free(path);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    CHECK(r.ResultCode == ADUC_Result_Download_Success);
    delete h;
}

TEST_CASE("Simulator: Download - get_update_file fails", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    mock_get_update_file_return = false;
    // Remove data file so we hit the no-data-file path for download
    char* path = GetSimulatorDataFilePath();
    remove(path);
    free(path);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    // With no data file, it defaults to Download_Success, but the file loop may fail
    // Actually, looking at the code: if no data file, it returns Download_Success immediately
    // before the loop. If data file exists, it enters the loop.
    CHECK(r.ResultCode == ADUC_Result_Download_Success);
    delete h;
}

TEST_CASE("Simulator: Download - with data file specifying success", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    const char* json =
        R"({"download":{"*":{"resultCode":500,"extendedResultCode":0,"resultDetails":"ok"}}})";
    char* path = write_sim_data(json);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    CHECK(r.ResultCode == 500);
    CHECK(r.ExtendedResultCode == 0);

    remove(path);
    free(path);
    delete h;
}

TEST_CASE("Simulator: Download - with data file specifying failure", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    const char* json =
        R"({"download":{"*":{"resultCode":0,"extendedResultCode":99999,"resultDetails":"fail"}}})";
    char* path = write_sim_data(json);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    CHECK(r.ResultCode == 0);
    CHECK(r.ExtendedResultCode == 99999);

    remove(path);
    free(path);
    delete h;
}

TEST_CASE("Simulator: Download - file-specific result in data", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    mock_target_filename = "specific-file.bin";
    const char* json =
        R"({"download":{"specific-file.bin":{"resultCode":501,"extendedResultCode":0,"resultDetails":"specific"}}})";
    char* path = write_sim_data(json);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    CHECK(r.ResultCode == 501);

    remove(path);
    free(path);
    delete h;
}

TEST_CASE("Simulator: Download - get_update_file fails with data", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    mock_get_update_file_return = false;
    const char* json =
        R"({"download":{"*":{"resultCode":500,"extendedResultCode":0,"resultDetails":"ok"}}})";
    char* path = write_sim_data(json);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    CHECK(r.ResultCode == ADUC_Result_Failure);

    remove(path);
    free(path);
    delete h;
}

/* =====================================================================
 * Install tests
 * ===================================================================== */

TEST_CASE("Simulator: Install - no data file, default success", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    char* path = GetSimulatorDataFilePath();
    remove(path);
    free(path);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Install(&wd);
    CHECK(r.ResultCode == ADUC_Result_Install_Success);
    delete h;
}

TEST_CASE("Simulator: Install - with data file", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    const char* json =
        R"({"install":{"resultCode":603,"extendedResultCode":0,"resultDetails":"skipped"}})";
    char* path = write_sim_data(json);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Install(&wd);
    CHECK(r.ResultCode == 603);

    remove(path);
    free(path);
    delete h;
}

/* =====================================================================
 * Apply tests
 * ===================================================================== */

TEST_CASE("Simulator: Apply - no data file, default success", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    char* path = GetSimulatorDataFilePath();
    remove(path);
    free(path);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Apply(&wd);
    CHECK(r.ResultCode == ADUC_Result_Apply_Success);
    delete h;
}

TEST_CASE("Simulator: Apply - with failure data", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    const char* json =
        R"({"apply":{"resultCode":0,"extendedResultCode":44444,"resultDetails":"failed"}})";
    char* path = write_sim_data(json);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Apply(&wd);
    CHECK(r.ResultCode == 0);
    CHECK(r.ExtendedResultCode == 44444);

    remove(path);
    free(path);
    delete h;
}

/* =====================================================================
 * Cancel tests
 * ===================================================================== */

TEST_CASE("Simulator: Cancel - no data file, default success", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    char* path = GetSimulatorDataFilePath();
    remove(path);
    free(path);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Cancel(&wd);
    CHECK(r.ResultCode == ADUC_Result_Cancel_Success);
    delete h;
}

TEST_CASE("Simulator: Cancel - with failure data", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    const char* json =
        R"({"cancel":{"resultCode":0,"extendedResultCode":55555,"resultDetails":"fail"}})";
    char* path = write_sim_data(json);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Cancel(&wd);
    CHECK(r.ResultCode == 0);
    CHECK(r.ExtendedResultCode == 55555);

    remove(path);
    free(path);
    delete h;
}

/* =====================================================================
 * IsInstalled tests
 * ===================================================================== */

TEST_CASE("Simulator: IsInstalled - no data file, default installed", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    char* path = GetSimulatorDataFilePath();
    remove(path);
    free(path);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->IsInstalled(&wd);
    // Default result code for IsInstalled is ADUC_Result_IsInstalled_Installed (900)
    CHECK(r.ResultCode == ADUC_Result_IsInstalled_Installed);
    delete h;
}

TEST_CASE("Simulator: IsInstalled - with installed result", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    mock_installed_criteria = "my-criteria";
    const char* json =
        R"({"isInstalled":{"my-criteria":{"resultCode":900,"extendedResultCode":0,"resultDetails":""}}})";
    char* path = write_sim_data(json);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->IsInstalled(&wd);
    CHECK(r.ResultCode == 900);

    remove(path);
    free(path);
    delete h;
}

TEST_CASE("Simulator: IsInstalled - with catch-all result", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    const char* json =
        R"({"isInstalled":{"*":{"resultCode":901,"extendedResultCode":0,"resultDetails":"not installed"}}})";
    char* path = write_sim_data(json);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->IsInstalled(&wd);
    CHECK(r.ResultCode == 901);

    remove(path);
    free(path);
    delete h;
}

TEST_CASE("Simulator: IsInstalled - null installed criteria, no data file", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    mock_installed_criteria = nullptr;
    char* path = GetSimulatorDataFilePath();
    remove(path);
    free(path);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->IsInstalled(&wd);
    // No data file -> returns default (ADUC_Result_IsInstalled_Installed)
    CHECK(r.ResultCode == ADUC_Result_IsInstalled_Installed);
    delete h;
}

/* =====================================================================
 * Backup tests
 * ===================================================================== */

TEST_CASE("Simulator: Backup - no data file, default success", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    char* path = GetSimulatorDataFilePath();
    remove(path);
    free(path);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Backup(&wd);
    CHECK(r.ResultCode == ADUC_Result_Backup_Success);
    delete h;
}

TEST_CASE("Simulator: Backup - with data", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    const char* json =
        R"({"backup":{"resultCode":1000,"extendedResultCode":0,"resultDetails":"ok"}})";
    char* path = write_sim_data(json);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Backup(&wd);
    CHECK(r.ResultCode == 1000);

    remove(path);
    free(path);
    delete h;
}

/* =====================================================================
 * Restore tests
 * ===================================================================== */

TEST_CASE("Simulator: Restore - no data file, default success", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    char* path = GetSimulatorDataFilePath();
    remove(path);
    free(path);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Restore(&wd);
    CHECK(r.ResultCode == ADUC_Result_Restore_Success);
    delete h;
}

TEST_CASE("Simulator: Restore - with data", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    const char* json =
        R"({"restore":{"resultCode":1100,"extendedResultCode":0,"resultDetails":"ok"}})";
    char* path = write_sim_data(json);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Restore(&wd);
    CHECK(r.ResultCode == 1100);

    remove(path);
    free(path);
    delete h;
}

/* =====================================================================
 * Download - multiple files
 * ===================================================================== */

TEST_CASE("Simulator: Download - multiple files with no data", "[simulator_handler]")
{
    mock_simulator_handler_reset();
    mock_update_files_count = 3;
    char* path = GetSimulatorDataFilePath();
    remove(path);
    free(path);

    auto wd = make_workflow_data();
    ContentHandler* h = SimulatorHandlerImpl::CreateContentHandler();
    ADUC_Result r = h->Download(&wd);
    CHECK(r.ResultCode == ADUC_Result_Download_Success);
    delete h;
}
