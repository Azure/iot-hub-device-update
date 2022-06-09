/**
 * @file hash_utils_ut.cpp
 * @brief Unit Tests for hash_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/hash_utils.h>

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <array>
#include <fstream>
#include <unordered_map>

// To generate file hashes:
// openssl dgst -binary -sha256 < test.bin  | openssl base64

class TestFile
{
public:
    TestFile(const TestFile&) = delete;
    TestFile& operator=(const TestFile&) = delete;
    TestFile(TestFile&&) = delete;
    TestFile& operator=(TestFile&&) = delete;

    TestFile()
    {
        // Generate a unique filename.
        int result = mkstemp(_filePath);
        REQUIRE(result != -1);
    }

    virtual ~TestFile()
    {
        REQUIRE(std::remove(Filename()) == 0);
    }

    virtual const uint8_t* GetData() const = 0;
    virtual const size_t GetDataByteLen() const = 0;
    virtual const char* GetDataHashBase64(SHAversion shaVersion) const = 0;

    const char* Filename() const
    {
        return _filePath;
    }

protected:
    // This can't be called from base class ctor, as it calls virtual functions.
    virtual void CreateFile()
    {
        (void)std::remove(Filename());
        std::ofstream file{ Filename(), std::ios::app | std::ios::binary };
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        file.write(reinterpret_cast<const char*>(GetData()), GetDataByteLen());
    }

private:
    char _filePath[ARRAY_SIZE("/tmp/tmpfileXXXXXX")] = "/tmp/tmpfileXXXXXX";
};

class SmallFile : public TestFile
{
public:
    SmallFile()
    {
        CreateFile();
    }

    const uint8_t* GetData() const override
    {
        return _data.data();
    }

    const size_t GetDataByteLen() const override
    {
        return _data.size() * sizeof(uint8_t);
    }

    const char* GetDataHashBase64(SHAversion shaVersion) const override
    {
        return _hashBase64Map.at(shaVersion).c_str();
    }

private:
    const std::array<uint8_t, 16> _data{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };

    // clang-format off
    const std::unordered_map<SHAversion, std::string> _hashBase64Map
    {
        { SHAversion::SHA1,   "VheLhqV/rCKJmplkGFwsyW59pYk=" },
        { SHAversion::SHA224, "Up1laovEE/71jaguG/Awjc/gQp3NgGh+aclGMw==" },
        { SHAversion::SHA256, "vkXLJgW/Nr695oSEGijw/UPGmFCj3OX+26aZKO46iZE=" },
        { SHAversion::SHA384, "yB35jZ5t6bhYoebroPGjo5nZjEQeZ+EGJgGAZIW7iRJe/VTMeN9fvOq8k818e6E7" },
        { SHAversion::SHA512, "2qKVvu1OLulMJAFbVq9ia08h759E8rPUD8QckJAKa/G0hnxDxXzaVNG2/Uhps/I87V4Lo8BdCxaA307H0HYkAw==" }
    };
    // clang-format on
};

TEST_CASE("ADUC_HashUtils_GetFileHash - SmallFile")
{
    SmallFile testFile;

    // clang-format off
    auto version = GENERATE( // NOLINT(google-build-using-namespace)
        SHAversion::SHA1,
        SHAversion::SHA224,
        SHAversion::SHA256,
        SHAversion::SHA384,
        SHAversion::SHA512);
    // clang-format on

    SECTION("Verify file hash")
    {
        INFO("SHAversion: " << version);
        char* hash = nullptr;
        REQUIRE(ADUC_HashUtils_GetFileHash(testFile.Filename(), version, &hash));
        CHECK_THAT(hash, Equals(testFile.GetDataHashBase64(version)));
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc)
        free(hash);
        hash = nullptr;
    }
}

TEST_CASE("ADUC_HashUtils_IsValidFileHash - SmallFile")
{
    SmallFile testFile;

    // clang-format off
    auto version = GENERATE( // NOLINT(google-build-using-namespace)
        SHAversion::SHA1,
        SHAversion::SHA224,
        SHAversion::SHA256,
        SHAversion::SHA384,
        SHAversion::SHA512);
    // clang-format on

    SECTION("Verify file hash")
    {
        INFO("SHAversion: " << version);
        REQUIRE(
            ADUC_HashUtils_IsValidFileHash(testFile.Filename(), testFile.GetDataHashBase64(version), version, true));
    }

    SECTION("Verify bad file hash")
    {
        INFO("SHAversion: " << version);
        REQUIRE_FALSE(ADUC_HashUtils_IsValidFileHash(
            testFile.Filename(), "xxXXXgW/Nr695oSEGijw/UPGmFCj3OX+26aZKO46iZE=", version, true));
    }
}

TEST_CASE("ADUC_HashUtils_IsValidBufferHash")
{
    SmallFile testFile;

    SECTION("Verify buffer hash")
    {
        REQUIRE(ADUC_HashUtils_IsValidBufferHash(
            testFile.GetData(),
            testFile.GetDataByteLen(),
            testFile.GetDataHashBase64(SHAversion::SHA256),
            SHAversion::SHA256));
    }

    SECTION("Verify bad buffer hash")
    {
        REQUIRE_FALSE(ADUC_HashUtils_IsValidBufferHash(
            testFile.GetData(),
            testFile.GetDataByteLen(),
            testFile.GetDataHashBase64(SHAversion::SHA256),
            SHAversion::SHA384));

        REQUIRE_FALSE(ADUC_HashUtils_IsValidBufferHash(
            testFile.GetData(),
            testFile.GetDataByteLen(),
            "xxXXXgW/Nr695oSEGijw/UPGmFCj3OX+26aZKO46iZE=",
            SHAversion::SHA256));
    }
}

TEST_CASE("ADUC_HashUtils_GetShaVersionForTypeString")
{
    SECTION("Valid case-sensitive type strings")
    {
        // clang-format off
        const std::unordered_map<std::string, SHAversion> validTypeStrings
        {
            { "sha1", SHAversion::SHA1 },
            { "sha224", SHAversion::SHA224 },
            { "sha256", SHAversion::SHA256 },
            { "sha384", SHAversion::SHA384 },
            { "sha512", SHAversion::SHA512 },
            // case-insensitive check
            { "sHa512", SHAversion::SHA512 }
        };
        // clang-format on

        for (const auto& it : validTypeStrings)
        {
            SHAversion algorithm;
            REQUIRE(ADUC_HashUtils_GetShaVersionForTypeString(it.first.c_str(), &algorithm));
            REQUIRE(algorithm == it.second);
        }
    }

    SECTION("Invalid type strings")
    {
        SHAversion algorithm;
        REQUIRE_FALSE(ADUC_HashUtils_GetShaVersionForTypeString("sha42", &algorithm));
    }
}

class LargeFile : public TestFile
{
public:
    LargeFile()
    {
        _data.resize(TestBufferSize);

        uint8_t n = 0;
        std::generate(_data.begin(), _data.end(), [&n]() mutable { return n++; });

        CreateFile();
    }

    const uint8_t* GetData() const override
    {
        return _data.data();
    }

    const size_t GetDataByteLen() const override
    {
        return _data.size() * sizeof(uint8_t);
    }

    const char* GetDataHashBase64(SHAversion shaVersion) const override
    {
        return _hashBase64Map.at(shaVersion).c_str();
    }

private:
    // > 512K read buffer
    static constexpr size_t TestBufferSize = 512 * 1024 + 4;
    std::vector<uint8_t> _data;

    // clang-format off
    const std::unordered_map<SHAversion, std::string> _hashBase64Map
    {
        { SHAversion::SHA1,   "bYdE/09ERVjkspyxa/EEC0ZR3tU=" },
        { SHAversion::SHA224, "VxOSDzeykzzJXxHLb9ea8U/IIwH5YEZd+kLBgg==" },
        { SHAversion::SHA256, "GFwFfnO+4hAqGZZWATWrAeej+KYKN6Gadwo7ovAGzeQ=" },
        { SHAversion::SHA384, "HD/4Q5ltgOg70vTsPYhu0K2V9v2F4hm2Tc8ry+D3T0E1ATFuZE8nkL++tdGYNb5Z" },
        { SHAversion::SHA512, "bl5Rqu9zNh6Atk2TgqX+Z1NsKvlWlomv0Gsh4w2+gNoUVEZ82PtAQXsAGYoVzRbXIkCg12VPytOIZFOrIWSLjw==" }
    };
    // clang-format on
};

TEST_CASE("ADUC_HashUtils_IsValidFileHash - LargeFile")
{
    LargeFile testFile;

    // clang-format off
    auto version = GENERATE( // NOLINT(google-build-using-namespace)
        SHAversion::SHA1,
        SHAversion::SHA224,
        SHAversion::SHA256,
        SHAversion::SHA384,
        SHAversion::SHA512);
    // clang-format on

    SECTION("Verify file hash")
    {
        INFO("SHAversion: " << version);
        REQUIRE(
            ADUC_HashUtils_IsValidFileHash(testFile.Filename(), testFile.GetDataHashBase64(version), version, true));
    }

    SECTION("Verify bad file hash")
    {
        INFO("SHAversion: " << version);
        REQUIRE_FALSE(ADUC_HashUtils_IsValidFileHash(
            testFile.Filename(), "xxXXXgW/Nr695oSEGijw/UPGmFCj3OX+26aZKO46iZE=", version, true));
    }
}

TEST_CASE("ADUC_HashUtils_GetFileHash - LargeFile")
{
    SmallFile testFile;

    // clang-format off
    auto version = GENERATE( // NOLINT(google-build-using-namespace)
        SHAversion::SHA1,
        SHAversion::SHA224,
        SHAversion::SHA256,
        SHAversion::SHA384,
        SHAversion::SHA512);
    // clang-format on

    SECTION("Verify file hash")
    {
        INFO("SHAversion: " << version);
        char* hash = nullptr;
        REQUIRE(ADUC_HashUtils_GetFileHash(testFile.Filename(), version, &hash));
        CHECK_THAT(hash, Equals(testFile.GetDataHashBase64(version)));
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc)
        free(hash);
        hash = nullptr;
    }
}
