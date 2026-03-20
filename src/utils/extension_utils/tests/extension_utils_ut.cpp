#include <aduc/extension_utils.h>
#include <aduc/parser_utils.h>
#include <aduc/types/update_content.h>
#include <catch2/catch_all.hpp>

#include <azure_c_shared_utility/strings.h>
#include <parson.h>

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

// ---------------------------------------------------------------------------
// ARM32 regression tests – validate the format strings used in
// RegisterExtension / RegisterHandlerExtension match their argument types.
// See commit b5fd2113 for details on the %lld → %ld fix.
// ---------------------------------------------------------------------------

static STRING_HANDLE BuildExtensionRegistrationJson(
    const char* extensionFilePath, long fileSize, const char* hash)
{
    return STRING_construct_sprintf(
        "{\n"
        "   \"fileName\":\"%s\",\n"
        "   \"sizeInBytes\":%ld,\n"
        "   \"hashes\": {\n"
        "        \"sha256\":\"%s\"\n"
        "   }\n"
        "}\n",
        extensionFilePath,
        fileSize,
        hash);
}

static STRING_HANDLE BuildHandlerRegistrationJson(
    const char* handlerFilePath, long fileSize, const char* hash, const char* handlerId)
{
    return STRING_construct_sprintf(
        "{\n"
        "   \"fileName\":\"%s\",\n"
        "   \"sizeInBytes\":%ld,\n"
        "   \"hashes\": {\n"
        "        \"sha256\":\"%s\"\n"
        "   },\n"
        "   \"handlerId\":\"%s\"\n"
        "}\n",
        handlerFilePath,
        fileSize,
        hash,
        handlerId);
}

TEST_CASE("RegisterExtension JSON format - regression test for ARM32 segfault")
{
    const char* filePath = "/var/lib/adu/extensions/sources/libmicrosoft_apt_1.so";
    const char* sha256 = "YWJjZGVmZzEyMzQ1Njc4OQ==";

    SECTION("Typical file size")
    {
        long fileSize = 331728L;

        STRING_HANDLE json = BuildExtensionRegistrationJson(filePath, fileSize, sha256);
        REQUIRE(json != NULL);

        JSON_Value* root = json_parse_string(STRING_c_str(json));
        REQUIRE(root != NULL);

        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != NULL);

        CHECK(strcmp(json_object_get_string(obj, "fileName"), filePath) == 0);
        CHECK(json_object_get_number(obj, "sizeInBytes") == static_cast<double>(fileSize));

        JSON_Object* hashes = json_object_get_object(obj, "hashes");
        REQUIRE(hashes != NULL);
        CHECK(strcmp(json_object_get_string(hashes, "sha256"), sha256) == 0);

        json_value_free(root);
        STRING_delete(json);
    }

    SECTION("Zero file size")
    {
        long fileSize = 0L;

        STRING_HANDLE json = BuildExtensionRegistrationJson(filePath, fileSize, sha256);
        REQUIRE(json != NULL);

        JSON_Value* root = json_parse_string(STRING_c_str(json));
        REQUIRE(root != NULL);

        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != NULL);

        CHECK(json_object_get_number(obj, "sizeInBytes") == 0.0);
        CHECK(strcmp(json_object_get_string(obj, "fileName"), filePath) == 0);

        JSON_Object* hashes = json_object_get_object(obj, "hashes");
        REQUIRE(hashes != NULL);
        CHECK(strcmp(json_object_get_string(hashes, "sha256"), sha256) == 0);

        json_value_free(root);
        STRING_delete(json);
    }

    SECTION("Large file size near LONG_MAX")
    {
        long fileSize = LONG_MAX;

        STRING_HANDLE json = BuildExtensionRegistrationJson(filePath, fileSize, sha256);
        REQUIRE(json != NULL);

        JSON_Value* root = json_parse_string(STRING_c_str(json));
        REQUIRE(root != NULL);

        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != NULL);

        CHECK(json_object_get_number(obj, "sizeInBytes") == static_cast<double>(LONG_MAX));
        CHECK(strcmp(json_object_get_string(obj, "fileName"), filePath) == 0);

        JSON_Object* hashes = json_object_get_object(obj, "hashes");
        REQUIRE(hashes != NULL);
        CHECK(strcmp(json_object_get_string(hashes, "sha256"), sha256) == 0);

        json_value_free(root);
        STRING_delete(json);
    }
}

TEST_CASE("RegisterHandlerExtension JSON format - regression test for ARM32 segfault")
{
    const char* filePath = "/var/lib/adu/extensions/sources/libmicrosoft_apt_1.so";
    const char* sha256 = "YWJjZGVmZzEyMzQ1Njc4OQ==";
    const char* handlerId = "microsoft/apt:1";

    SECTION("Typical file size")
    {
        long fileSize = 331728L;

        STRING_HANDLE json = BuildHandlerRegistrationJson(filePath, fileSize, sha256, handlerId);
        REQUIRE(json != NULL);

        JSON_Value* root = json_parse_string(STRING_c_str(json));
        REQUIRE(root != NULL);

        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != NULL);

        CHECK(strcmp(json_object_get_string(obj, "fileName"), filePath) == 0);
        CHECK(json_object_get_number(obj, "sizeInBytes") == static_cast<double>(fileSize));
        CHECK(strcmp(json_object_get_string(obj, "handlerId"), handlerId) == 0);

        JSON_Object* hashes = json_object_get_object(obj, "hashes");
        REQUIRE(hashes != NULL);
        CHECK(strcmp(json_object_get_string(hashes, "sha256"), sha256) == 0);

        json_value_free(root);
        STRING_delete(json);
    }

    SECTION("Large file size near LONG_MAX")
    {
        long fileSize = LONG_MAX;

        STRING_HANDLE json = BuildHandlerRegistrationJson(filePath, fileSize, sha256, handlerId);
        REQUIRE(json != NULL);

        JSON_Value* root = json_parse_string(STRING_c_str(json));
        REQUIRE(root != NULL);

        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != NULL);

        CHECK(json_object_get_number(obj, "sizeInBytes") == static_cast<double>(LONG_MAX));
        CHECK(strcmp(json_object_get_string(obj, "fileName"), filePath) == 0);
        CHECK(strcmp(json_object_get_string(obj, "handlerId"), handlerId) == 0);

        JSON_Object* hashes = json_object_get_object(obj, "hashes");
        REQUIRE(hashes != NULL);
        CHECK(strcmp(json_object_get_string(hashes, "sha256"), sha256) == 0);

        json_value_free(root);
        STRING_delete(json);
    }
}
