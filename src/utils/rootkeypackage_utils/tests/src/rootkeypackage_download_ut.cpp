/**
 * @file rootkeypackage_utils_ut.cpp
 * @brief Unit Tests for rootkeypackage_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/file_test_utils.hpp>
#include <aduc/rootkeypackage_utils.h>
#include <aduc/system_utils.h>
#include <azure_c_shared_utility/strings.h>
#include <catch2/catch.hpp>
#include <fstream>
#include <sstream>

using Catch::Matchers::Equals;

class TestCaseFixture
{
public:
    TestCaseFixture() : m_testPath{ ADUC_SystemUtils_GetTemporaryPathName() }
    {
        m_testPath += "/rootkeypackage_download_ut";

        (void)ADUC_SystemUtils_RmDirRecursive(m_testPath.c_str());
        (void)ADUC_SystemUtils_MkDirRecursiveDefault(m_testPath.c_str());
    }

    ~TestCaseFixture()
    {
        (void)ADUC_SystemUtils_RmDirRecursive(m_testPath.c_str());
    }

    const char* TestPath() const
    {
        return m_testPath.c_str();
    }

    TestCaseFixture(const TestCaseFixture&) = delete;
    TestCaseFixture& operator=(const TestCaseFixture&) = delete;
    TestCaseFixture(TestCaseFixture&&) = delete;
    TestCaseFixture& operator=(TestCaseFixture&&) = delete;

    static ADUC_Result TestRootKeyPkgDownload(const char* rootKeyPkgUrl, const char* targetFilePath)
    {
        ADUC_Result result{ ADUC_GeneralResult_Failure, 0 };
        try
        {
            std::ofstream ofs;
            ofs.open(targetFilePath);
            REQUIRE((ofs.rdstate() & std::ofstream::badbit) == 0);
            ofs << "rootKeyPkgUrl=" << rootKeyPkgUrl << ", targetFilePath=" << targetFilePath;
            ofs.close();

            result.ResultCode = ADUC_GeneralResult_Success;
        }
        catch (...)
        {
        }

        return result;
    }

private:
    std::string m_testPath{};
};

TEST_CASE_METHOD(TestCaseFixture, "RootKeyPackageUtils_DownloadPackage")
{
    ADUC_RootKeyPkgDownloaderInfo downloaderInfo{
        /* .name = */ "test-downloader",
        /* .downloadFn = */ TestCaseFixture::TestRootKeyPkgDownload,
        /* .downloadBaseDir = */ TestPath(),
    };

    const std::string rootKeyPkgUrl{ "http://localhost:8080/path/fake.json" };
    const std::string workflowId{ "afc5140e-b253-4f37-a810-d9593bd7fc0c" };
    STRING_HANDLE downloadedFile = nullptr;
    ADUC_Result result{ ADUC_RootKeyPackageUtils_DownloadPackage(
        "http://localhost:8080/path/fake.json", workflowId.c_str(), &downloaderInfo, &downloadedFile) };

    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    std::stringstream ssExpectedDownloadFile{};
    ssExpectedDownloadFile << TestPath() << "/" << workflowId << "/fake.json";
    const std::string expectedDownloadFilePath{ ssExpectedDownloadFile.str() };
    CHECK_THAT(STRING_c_str(downloadedFile), Equals(expectedDownloadFilePath));

    std::stringstream ssExpectedContent{};
    ssExpectedContent << "rootKeyPkgUrl=" << rootKeyPkgUrl << ", targetFilePath=" << expectedDownloadFilePath;
    const std::string expectedDownloadContent{ ssExpectedContent.str() };
    CHECK_THAT(
        aduc::FileTestUtils_slurpFile(std::string(STRING_c_str(downloadedFile))), Equals(expectedDownloadContent));
}
