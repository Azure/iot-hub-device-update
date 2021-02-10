#include "aduc/adu_core_exports.h"
#include "aduc/apt_handler.hpp"
#include "aduc/apt_parser.hpp"
#include "aduc/content_handler_factory.hpp"
#include "aduc/system_utils.h"
#include <catch2/catch.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <parson.h>
#include <sstream>
#include <string>
#include <sys/stat.h>

struct JSONValueDeleter
{
    void operator()(JSON_Value* value)
    {
        json_value_free(value);
    }
};

using AutoFreeJsonValue_t = std::unique_ptr<JSON_Value, JSONValueDeleter>;

const char* aptTestJSONStringInstallLibCurlAndAptDoc =
    R"({)"
    R"(  "name":"com-microsoft-eds-adu-testapt", )"
    R"(  "version":"1.0.0", )"
    R"(  "packages": [ )"
    R"(     {)"
    R"(       "name":"libcurl4-doc" )"
    R"(     },)"
    R"(     {)"
    R"(       "name":"apt-doc", "version":"1.6.1" )"
    R"(     })"
    R"(   ])"
    R"(})";

const char* aptTestJSONStringUpgradeToLatestAptDoc =
    R"({)"
    R"(  "name":"com-microsoft-eds-adu-testapt", )"
    R"(  "version":"1.0.1", )"
    R"(  "packages": [ )"
    R"(     {)"
    R"(       "name":"apt-doc" )"
    R"(     })"
    R"(   ])"
    R"(})";

const char* aptTestJSONStringWithBogusPackage =
    R"({)"
    R"(  "name":"com-microsoft-eds-adu-aptwithboguspackage", )"
    R"(  "version":"1.0.0", )"
    R"(  "packages": [ )"
    R"(    {)"
    R"(      "name":"some-package-foo-xyz" )"
    R"(    })"
    R"(  ])"
    R"(})";

const char* aptTestJSONStringBadVersion =
    R"({)"
    R"(  "name":"com-microsoft-eds-adu-aptwithbadversion", )"
    R"(  "version":"1.0.0", )"
    R"(  "packages": [ )"
    R"(    {)"
    R"(      "name":"libcurl-dev", "version":"=1.0.8-2" )"
    R"(    })"
    R"(  ])"
    R"(})";

TEST_CASE("APT handler prepare success")
{
    const std::string& workFolder = "/tmp";
    const std::string& fileName = "test.json";
    const std::string& testFile = workFolder + "/" + fileName;
    std::fstream file{ testFile, std::ios::out };

    file << aptTestJSONStringInstallLibCurlAndAptDoc;
    file.close();

    auto contentHandler =
        std::unique_ptr<ContentHandler>{ AptHandlerImpl::CreateContentHandler(workFolder, fileName) };
    CHECK(contentHandler);

    ADUC_PrepareInfo info = {};
    char updateType[16] = "microsoft/apt:1";
    char updateTypeName[14] = "microsoft/apt";
    info.updateType = updateType;
    info.updateTypeName = updateTypeName;

    SECTION("Prepare Success")
    {
        info.fileCount = 1;
        info.updateTypeVersion = 1;
        ADUC_Result result = contentHandler->Prepare(&info);
        CHECK(result.ResultCode == ADUC_PrepareResult_Success);
    }

    SECTION("Prepare Fail with Wrong File Count")
    {
        info.fileCount = 2;
        info.updateTypeVersion = 1;
        ADUC_Result result = contentHandler->Prepare(&info);
        CHECK(result.ResultCode == ADUC_PrepareResult_Failure);
        CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT);
    }

    SECTION("APT handler prepare wrong version")
    {
        info.fileCount = 1;
        info.updateTypeVersion = 2;
        ADUC_Result result = contentHandler->Prepare(&info);
        CHECK(result.ResultCode == ADUC_PrepareResult_Failure);
        CHECK(result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_VERSION);
    }

    REQUIRE(std::remove(testFile.c_str()) == 0);
}

TEST_CASE("APT handler download missing package test",  "[!hide][functional_test]")
{
    const std::string& workFolder = "/tmp";
    const std::string& fileName = "test.json";
    const std::string& testFile = workFolder + "/" + fileName;
    std::fstream file{ testFile, std::ios::out };

    file << aptTestJSONStringWithBogusPackage;
    file.close();

    auto contentHandler =
        std::unique_ptr<ContentHandler>{ AptHandlerImpl::CreateContentHandler(workFolder, fileName) };
    CHECK(contentHandler);
    ADUC_Result result = contentHandler->Download();
    CHECK(result.ResultCode == ADUC_DownloadResult_Failure);

    REQUIRE(std::remove(testFile.c_str()) == 0);
}

TEST_CASE("APT Handler bad version number test",  "[!hide][functional_test]")
{
    const std::string& workFolder = "/tmp";
    const std::string& fileName = "test.json";
    const std::string& testFile = workFolder + "/" + fileName;
    std::fstream file{ testFile, std::ios::out };

    file << aptTestJSONStringBadVersion;
    file.close();

    auto contentHandler =
        std::unique_ptr<ContentHandler>{ AptHandlerImpl::CreateContentHandler(workFolder, fileName) };
    CHECK(contentHandler);
    ADUC_Result result = contentHandler->Download();
    CHECK(result.ResultCode == ADUC_DownloadResult_Failure);

    REQUIRE(std::remove(testFile.c_str()) == 0);
}

TEST_CASE("APT Handler download tests", "[!hide][functional_test]")
{
    const std::string& workFolder = "/tmp";
    const std::string& fileName = "test.json";
    const std::string& testFile = workFolder + "/" + fileName;
    std::fstream file{ testFile, std::ios::out };

    file << aptTestJSONStringInstallLibCurlAndAptDoc;
    file.close();

    auto contentHandler =
        std::unique_ptr<ContentHandler>{ AptHandlerImpl::CreateContentHandler(workFolder, fileName) };
    CHECK(contentHandler);
    ADUC_Result result = contentHandler->Download();
    CHECK(result.ResultCode == ADUC_DownloadResult_Success);

    REQUIRE(std::remove(testFile.c_str()) == 0);
}

TEST_CASE("APT Handler install test", "[!hide][functional_test]")
{
    const std::string& workFolder = "/tmp";
    const std::string& fileName = "test.json";
    const std::string& testFile = workFolder + "/" + fileName;
    std::fstream file{ testFile, std::ios::out };

    file << aptTestJSONStringInstallLibCurlAndAptDoc;
    file.close();

    auto contentHandler =
        std::unique_ptr<ContentHandler>{ AptHandlerImpl::CreateContentHandler(workFolder, fileName) };
    CHECK(contentHandler);
    ADUC_Result result = contentHandler->Install();
    CHECK(result.ResultCode == ADUC_InstallResult_Success);

    REQUIRE(std::remove(testFile.c_str()) == 0);
}

TEST_CASE("APT Handler apply test", "[!hide][functional_test]")
{
    const std::string& workFolder = "/tmp";
    const std::string& fileName = "test.json";
    const std::string& testFile = workFolder + "/" + fileName;
    std::fstream file{ testFile, std::ios::out };

    file << aptTestJSONStringInstallLibCurlAndAptDoc;
    file.close();

    auto contentHandler =
        std::unique_ptr<ContentHandler>{ AptHandlerImpl::CreateContentHandler(workFolder, fileName) };
    CHECK(contentHandler);
    ADUC_Result result = contentHandler->Apply();
    CHECK(result.ResultCode == ADUC_ApplyResult_Success);

    REQUIRE(std::remove(testFile.c_str()) == 0);
}

TEST_CASE("APT Handler update apt-doc tests", "[!hide][functional_test]")
{
    const std::string& workFolder = "/tmp";
    const std::string& fileName = "test.json";
    const std::string& testFile = workFolder + "/" + fileName;
    std::fstream file{ testFile, std::ios::out };

    file << aptTestJSONStringUpgradeToLatestAptDoc;
    file.close();

    auto contentHandler =
        std::unique_ptr<ContentHandler>{ AptHandlerImpl::CreateContentHandler(workFolder, fileName) };
    CHECK(contentHandler);

    ADUC_Result result = contentHandler->Download();
    CHECK(result.ResultCode == ADUC_DownloadResult_Success);

    result = contentHandler->Install();
    CHECK(result.ResultCode == ADUC_InstallResult_Success);

    result = contentHandler->Apply();
    CHECK(result.ResultCode == ADUC_ApplyResult_Success);

    REQUIRE(std::remove(testFile.c_str()) == 0);
}

TEST_CASE("APT_Handler_IsInstalled_test")
{
    AptHandlerImpl::RemoveAllInstalledCriteria();
    // Persist foo
    const char* installedCriteria_foo = "contoso-iot-edge-6.1.0.19";
    ADUC_Result isInstalled = AptHandlerImpl::GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode != ADUC_IsInstalledResult_Installed);

    bool persisted = AptHandlerImpl::PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(persisted);

    isInstalled = AptHandlerImpl::GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode == ADUC_IsInstalledResult_Installed);

    // Persist bar
    const char* installedCriteria_bar = "bar.1.0.1";
    isInstalled = AptHandlerImpl::GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(isInstalled.ResultCode != ADUC_IsInstalledResult_Installed);

    persisted = AptHandlerImpl::PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(persisted);

    isInstalled = AptHandlerImpl::GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(isInstalled.ResultCode == ADUC_IsInstalledResult_Installed);

    // Remove foo
    bool removed = AptHandlerImpl::RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(removed);

    isInstalled = AptHandlerImpl::GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode != ADUC_IsInstalledResult_Installed);

    // Remove bar
    removed = AptHandlerImpl::RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(removed);

    isInstalled = AptHandlerImpl::GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_bar);
    CHECK(isInstalled.ResultCode != ADUC_IsInstalledResult_Installed);
}

TEST_CASE("APT_Handler_RemoveIsInstalledTwice_test")
{
    AptHandlerImpl::RemoveAllInstalledCriteria();

    // Ensure foo doesn't exist.
    const char* installedCriteria_foo = "contoso-iot-edge-6.1.0.19";
    ADUC_Result isInstalled = AptHandlerImpl::GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode != ADUC_IsInstalledResult_Installed);

    // Persist foo.
    bool persisted = AptHandlerImpl::PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(persisted);

    // Foo is installed.
    isInstalled = AptHandlerImpl::GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(isInstalled.ResultCode == ADUC_IsInstalledResult_Installed);

    // Remove foo should succeeded.
    bool removed = AptHandlerImpl::RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(removed);

    // Regression test: 2nd remove should succeeded, with no infinite loop.
    removed = AptHandlerImpl::RemoveInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria_foo);
    CHECK(removed);
}