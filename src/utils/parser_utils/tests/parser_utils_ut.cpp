
#include "aduc/hash_utils.h"
#include "aduc/parser_utils.h"
#include <aduc/adu_types.h>
#include <aduc/types/hash.h>
#include <catch2/catch_all.hpp>

#include <string>

using Catch::Matchers::Equals;

TEST_CASE("ADUC_FileEntity_Init")
{
    SECTION("Deep copies fields")
    {
        ADUC_FileEntity fileEntity{};

        // Don't use calloc_wrapper as ADUC_Hash_FreeArray below will also free the hash
        ADUC_Hash* hash = static_cast<ADUC_Hash*>(calloc(1, sizeof(ADUC_Hash)));

        REQUIRE_FALSE(hash == nullptr);
        REQUIRE(ADUC_Hash_Init(hash, "hashvalue", "sha256"));

        char fileId[] = "abcdefg123456789";
        char targetFileName[] = "someFileName.ext";
        char downloadUri[] = "http://somehost.li/path/to/someFileName.ext";
        char arguments[] = "";

        REQUIRE(ADUC_FileEntity_Init(
            &fileEntity,
            fileId,
            targetFileName,
            downloadUri,
            arguments,
            hash,
            1, /* hashCount */
            1234567 /* sizeInBytes */));

        auto check_ptr_false = [](void* p, void* q) { CHECK_FALSE(p == q); };

        check_ptr_false(fileEntity.FileId, fileId);
        CHECK_THAT(fileEntity.FileId, Equals(fileId));

        check_ptr_false(fileEntity.TargetFilename, targetFileName);
        CHECK_THAT(fileEntity.TargetFilename, Equals(targetFileName));

        check_ptr_false(fileEntity.Arguments, arguments);
        CHECK_THAT(fileEntity.Arguments, Equals(arguments));

        check_ptr_false(fileEntity.DownloadUri, downloadUri);
        CHECK_THAT(fileEntity.DownloadUri, Equals(downloadUri));

        REQUIRE_FALSE(fileEntity.Hash == nullptr);
        check_ptr_false(fileEntity.Hash, hash);

        REQUIRE_FALSE(fileEntity.Hash->value == nullptr);
        check_ptr_false(fileEntity.Hash->value, hash->value);
        CHECK_THAT(fileEntity.Hash->value, Equals(hash->value));

        REQUIRE_FALSE(fileEntity.Hash->type == nullptr);
        check_ptr_false(fileEntity.Hash->type, hash->type);
        CHECK_THAT(fileEntity.Hash->type, Equals(hash->type));

        CHECK(fileEntity.HashCount == 1);
        CHECK(fileEntity.SizeInBytes == 1234567);

        // cleanup
        ADUC_FileEntity_Uninit(&fileEntity);
        ADUC_Hash_FreeArray(1, hash);
        hash = nullptr;
    }

    SECTION("Returns false for invalid required arguments")
    {
        ADUC_FileEntity fileEntity{};
        ADUC_Hash hash{};
        REQUIRE(ADUC_Hash_Init(&hash, "hashvalue", "sha256"));

        CHECK_FALSE(ADUC_FileEntity_Init(nullptr, "id", "file", "uri", "", &hash, 1, 1));
        CHECK_FALSE(ADUC_FileEntity_Init(&fileEntity, nullptr, "file", "uri", "", &hash, 1, 1));
        CHECK_FALSE(ADUC_FileEntity_Init(&fileEntity, "id", nullptr, "uri", "", &hash, 1, 1));
        CHECK_FALSE(ADUC_FileEntity_Init(&fileEntity, "id", "file", "uri", "", nullptr, 1, 1));

        ADUC_Hash_UnInit(&hash);
    }

    SECTION("Allows null downloadUri and arguments")
    {
        ADUC_FileEntity fileEntity{};
        ADUC_Hash hash{};
        REQUIRE(ADUC_Hash_Init(&hash, "hashvalue", "sha256"));

        REQUIRE(ADUC_FileEntity_Init(&fileEntity, "id", "file", nullptr, nullptr, &hash, 1, 42));
        CHECK(fileEntity.DownloadUri == nullptr);
        CHECK(fileEntity.Arguments == nullptr);
        CHECK(fileEntity.SizeInBytes == 42);

        ADUC_FileEntity_Uninit(&fileEntity);
        ADUC_Hash_UnInit(&hash);
    }
}

TEST_CASE("ADUC_HashArray_AllocAndInit")
{
    SECTION("Parses hash object")
    {
        JSON_Value* root = json_parse_string("{\"sha256\":\"abc\",\"sha1\":\"def\"}");
        REQUIRE(root != nullptr);
        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != nullptr);

        size_t hashCount = 0;
        ADUC_Hash* hashes = ADUC_HashArray_AllocAndInit(obj, &hashCount);
        REQUIRE(hashes != nullptr);
        CHECK(hashCount == 2);

        ADUC_Hash_FreeArray(hashCount, hashes);
        json_value_free(root);
    }

    SECTION("Fails with null count pointer")
    {
        JSON_Value* root = json_parse_string("{\"sha256\":\"abc\"}");
        REQUIRE(root != nullptr);
        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != nullptr);

        CHECK(ADUC_HashArray_AllocAndInit(obj, nullptr) == nullptr);
        json_value_free(root);
    }

    SECTION("Fails for empty object")
    {
        JSON_Value* root = json_parse_string("{}");
        REQUIRE(root != nullptr);
        JSON_Object* obj = json_value_get_object(root);
        REQUIRE(obj != nullptr);

        size_t hashCount = 99;
        ADUC_Hash* hashes = ADUC_HashArray_AllocAndInit(obj, &hashCount);
        CHECK(hashes == nullptr);
        CHECK(hashCount == 0);

        json_value_free(root);
    }
}

TEST_CASE("ADUC_JSON_GetUpdateManifestRoot")
{
    SECTION("Returns parsed updateManifest object")
    {
        JSON_Value* action = json_parse_string(
            "{\"updateManifest\":\"{\\\"updateId\\\":{\\\"provider\\\":\\\"p\\\",\\\"name\\\":\\\"n\\\",\\\"version\\\":\\\"v\\\"}}\"}");
        REQUIRE(action != nullptr);

        JSON_Value* manifest = ADUC_JSON_GetUpdateManifestRoot(action);
        REQUIRE(manifest != nullptr);
        REQUIRE(json_value_get_object(manifest) != nullptr);

        json_value_free(manifest);
        json_value_free(action);
    }

    SECTION("Returns null when updateManifest is missing")
    {
        JSON_Value* action = json_parse_string("{\"foo\":\"bar\"}");
        REQUIRE(action != nullptr);
        CHECK(ADUC_JSON_GetUpdateManifestRoot(action) == nullptr);
        json_value_free(action);
    }
}

TEST_CASE("ADUC_Json_GetUpdateId")
{
    SECTION("Parses valid updateId")
    {
        JSON_Value* action = json_parse_string(
            "{\"updateManifest\":\"{\\\"updateId\\\":{\\\"provider\\\":\\\"prov\\\",\\\"name\\\":\\\"name\\\",\\\"version\\\":\\\"1.0\\\"}}\"}");
        REQUIRE(action != nullptr);

        ADUC_UpdateId* updateId = nullptr;
        REQUIRE(ADUC_Json_GetUpdateId(action, &updateId));
        REQUIRE(updateId != nullptr);
        CHECK(std::string(updateId->Provider) == "prov");
        CHECK(std::string(updateId->Name) == "name");
        CHECK(std::string(updateId->Version) == "1.0");

        ADUC_UpdateId_UninitAndFree(updateId);
        json_value_free(action);
    }

    SECTION("Fails for missing updateId fields")
    {
        JSON_Value* action = json_parse_string(
            "{\"updateManifest\":\"{\\\"updateId\\\":{\\\"provider\\\":\\\"prov\\\"}}\"}");
        REQUIRE(action != nullptr);

        ADUC_UpdateId* updateId = reinterpret_cast<ADUC_UpdateId*>(0x1);
        CHECK_FALSE(ADUC_Json_GetUpdateId(action, &updateId));
        CHECK(updateId == nullptr);

        json_value_free(action);
    }
}

TEST_CASE("ADUC_UpdateId_AllocAndInit")
{
    SECTION("Allocates and sets all fields")
    {
        ADUC_UpdateId* updateId = ADUC_UpdateId_AllocAndInit("prov", "name", "2.0");
        REQUIRE(updateId != nullptr);
        CHECK(std::string(updateId->Provider) == "prov");
        CHECK(std::string(updateId->Name) == "name");
        CHECK(std::string(updateId->Version) == "2.0");
        ADUC_UpdateId_UninitAndFree(updateId);
    }

    SECTION("Returns null for invalid arguments")
    {
        CHECK(ADUC_UpdateId_AllocAndInit(nullptr, "name", "1") == nullptr);
        CHECK(ADUC_UpdateId_AllocAndInit("prov", nullptr, "1") == nullptr);
        CHECK(ADUC_UpdateId_AllocAndInit("prov", "name", nullptr) == nullptr);
    }
}
