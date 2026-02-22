#include <aduc/extension_utils.h>
#include <aduc/parser_utils.h>
#include <aduc/types/update_content.h>
#include <catch2/catch_all.hpp>

#include <cstdio>
#include <fstream>
#include <string>

namespace
{
std::string CreateTempJsonPath(const std::string& suffix)
{
    return "/tmp/adu_extension_utils_ut_" + suffix + ".json";
}

void WriteTextFile(const std::string& path, const std::string& content)
{
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    REQUIRE(file.is_open());
    file << content;
    file.close();
}
}

TEST_CASE("GetExtensionFileEntity parses valid extension registration")
{
    const std::string filePath = CreateTempJsonPath("valid");
    WriteTextFile(
        filePath,
        "{\n"
        "  \"fileName\": \"/usr/lib/adu/extensions/handler.so\",\n"
        "  \"hashes\": {\n"
        "    \"sha256\": \"abc123hash\"\n"
        "  }\n"
        "}\n");

    ADUC_FileEntity fileEntity{};

    const bool ok = GetExtensionFileEntity(filePath.c_str(), &fileEntity);
    CHECK(ok);
    REQUIRE(fileEntity.TargetFilename != nullptr);
    CHECK(std::string(fileEntity.TargetFilename) == "/usr/lib/adu/extensions/handler.so");
    REQUIRE(fileEntity.Hash != nullptr);
    CHECK(fileEntity.HashCount == 1);
    CHECK(std::string(fileEntity.Hash[0].type) == "sha256");
    CHECK(std::string(fileEntity.Hash[0].value) == "abc123hash");

    ADUC_FileEntity_Uninit(&fileEntity);
    std::remove(filePath.c_str());
}

TEST_CASE("GetExtensionFileEntity fails for missing file")
{
    ADUC_FileEntity fileEntity{};

    const bool ok = GetExtensionFileEntity("/tmp/adu_extension_utils_does_not_exist.json", &fileEntity);
    CHECK_FALSE(ok);
    CHECK(fileEntity.TargetFilename == nullptr);
    CHECK(fileEntity.Hash == nullptr);
    CHECK(fileEntity.HashCount == 0);
}

TEST_CASE("GetExtensionFileEntity fails for malformed json")
{
    const std::string filePath = CreateTempJsonPath("malformed");
    WriteTextFile(filePath, "{\n  \"fileName\": \"/tmp/handler.so\",\n");

    ADUC_FileEntity fileEntity{};

    const bool ok = GetExtensionFileEntity(filePath.c_str(), &fileEntity);
    CHECK_FALSE(ok);
    CHECK(fileEntity.TargetFilename == nullptr);
    CHECK(fileEntity.Hash == nullptr);

    std::remove(filePath.c_str());
}

TEST_CASE("GetExtensionFileEntity fails when hashes object is missing")
{
    const std::string filePath = CreateTempJsonPath("missing_hashes");
    WriteTextFile(filePath, "{\n  \"fileName\": \"/tmp/handler.so\"\n}\n");

    ADUC_FileEntity fileEntity{};

    const bool ok = GetExtensionFileEntity(filePath.c_str(), &fileEntity);
    CHECK_FALSE(ok);
    CHECK(fileEntity.TargetFilename == nullptr);
    CHECK(fileEntity.Hash == nullptr);

    std::remove(filePath.c_str());
}

TEST_CASE("GetExtensionFileEntity fails when fileName is missing")
{
    const std::string filePath = CreateTempJsonPath("missing_filename");
    WriteTextFile(
        filePath,
        "{\n"
        "  \"hashes\": {\n"
        "    \"sha256\": \"abc123hash\"\n"
        "  }\n"
        "}\n");

    ADUC_FileEntity fileEntity{};

    const bool ok = GetExtensionFileEntity(filePath.c_str(), &fileEntity);
    CHECK_FALSE(ok);
    CHECK(fileEntity.TargetFilename == nullptr);
    CHECK(fileEntity.Hash == nullptr);

    std::remove(filePath.c_str());
}

TEST_CASE("RegisterExtension validates required arguments")
{
    CHECK_FALSE(RegisterExtension(nullptr, "/tmp/extension.so"));
    CHECK_FALSE(RegisterExtension("", "/tmp/extension.so"));
    CHECK_FALSE(RegisterExtension("/tmp/extensions", nullptr));
    CHECK_FALSE(RegisterExtension("/tmp/extensions", ""));
}

TEST_CASE("Register handler wrappers reject invalid identifiers")
{
    CHECK_FALSE(RegisterUpdateContentHandler(nullptr, "/tmp/handler.so"));
    CHECK_FALSE(RegisterUpdateContentHandler("", "/tmp/handler.so"));
    CHECK_FALSE(RegisterUpdateContentHandler("microsoft/apt:1", nullptr));
    CHECK_FALSE(RegisterUpdateContentHandler("microsoft/apt:1", ""));
    CHECK_FALSE(RegisterDownloadHandler(nullptr, "/tmp/downloader.so"));
    CHECK_FALSE(RegisterDownloadHandler("", "/tmp/downloader.so"));
    CHECK_FALSE(RegisterDownloadHandler("microsoft/delta:1", nullptr));
    CHECK_FALSE(RegisterDownloadHandler("microsoft/delta:1", ""));
}

TEST_CASE("GetDownloadHandlerFileEntity rejects invalid identifiers")
{
    ADUC_FileEntity fileEntity{};

    CHECK_FALSE(GetDownloadHandlerFileEntity(nullptr, &fileEntity));
    CHECK_FALSE(GetDownloadHandlerFileEntity("", &fileEntity));
    CHECK_FALSE(GetDownloadHandlerFileEntity("microsoft/delta:1", nullptr));
}

TEST_CASE("Register extension wrappers validate arguments")
{
    CHECK_FALSE(RegisterComponentEnumeratorExtension(nullptr));
    CHECK_FALSE(RegisterComponentEnumeratorExtension(""));

    CHECK_FALSE(RegisterContentDownloaderExtension(nullptr));
    CHECK_FALSE(RegisterContentDownloaderExtension(""));

    CHECK_FALSE(RegisterExtension(nullptr, "/tmp/x.so"));
    CHECK_FALSE(RegisterExtension("", "/tmp/x.so"));
    CHECK_FALSE(RegisterExtension("/tmp/extensions", nullptr));
    CHECK_FALSE(RegisterExtension("/tmp/extensions", ""));
}
