
#include "aduc/hash_utils.h"
#include "aduc/parser_utils.h"
#include <aduc/types/hash.h>
#include <catch2/catch.hpp>

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
}
