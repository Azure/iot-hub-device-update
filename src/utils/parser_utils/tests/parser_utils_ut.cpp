
#include "aduc/parser_utils.h"
#include <aduc/calloc_wrapper.hpp>
#include "aduc/hash_utils.h"
#include <aduc/types/hash.h>
#include <catch2/catch.hpp>
#include <string>

using ADUC::StringUtils::calloc_wrapper;

TEST_CASE("ADUC_FileEntity_Init")
{
    SECTION("Deep copies fields")
    {
        calloc_wrapper<ADUC_FileEntity> fileEntity { static_cast<ADUC_FileEntity*>(calloc(1, sizeof(ADUC_FileEntity))) };
        REQUIRE_FALSE(fileEntity.is_null());

        // Don't use calloc_wrapper as ADUC_Hash_FreeArray below will also free the hash
        ADUC_Hash* hash = static_cast<ADUC_Hash*>(calloc(1, sizeof(ADUC_Hash)));

        REQUIRE_FALSE(hash == nullptr);
        REQUIRE(ADUC_Hash_Init(hash, "hashvalue", "sha256"));

        const std::string fileId{ "abcdefg123456789" };
        const std::string targetFileName{ "someFileName.ext" };
        const std::string downloadUri{ "http://somehost.li/path/to/someFileName.ext" };
        const std::string arguments{ "" };

        REQUIRE(ADUC_FileEntity_Init(
            fileEntity.get(),
            fileId.c_str(),
            targetFileName.c_str(),
            downloadUri.c_str(),
            arguments.c_str(),
            hash,
            1, /* hashCount */
            1234567 /* sizeInBytes */));

        // pointer memory checks
        CHECK_FALSE(&fileEntity->FileId[0] == &fileId.c_str()[0]);
        CHECK_FALSE(&fileEntity->TargetFilename[0] == &targetFileName.c_str()[0]);
        CHECK_FALSE(&fileEntity->Arguments[0] == &arguments.c_str()[0]);
        CHECK_FALSE(&fileEntity->DownloadUri[0] == &downloadUri.c_str()[0]);
        CHECK_FALSE(fileEntity->Hash == nullptr);
        CHECK_FALSE(fileEntity->Hash == hash);
        CHECK_FALSE(fileEntity->Hash->value == nullptr);
        CHECK_FALSE(static_cast<void*>(fileEntity->Hash->value) == static_cast<void*>(hash->value));
        CHECK_FALSE(fileEntity->Hash->type == nullptr);
        CHECK_FALSE(static_cast<void*>(fileEntity->Hash->type) == static_cast<void*>(hash->type));

        // non-memory checks
        CHECK(fileEntity->HashCount == 1);
        CHECK(fileEntity->SizeInBytes == 1234567);

        // cleanup
        ADUC_FileEntity_Uninit(fileEntity.get());
        ADUC_Hash_FreeArray(1, hash);
        hash = nullptr;
    }
}
