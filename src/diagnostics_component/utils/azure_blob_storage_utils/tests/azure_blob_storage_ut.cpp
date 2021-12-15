/**
 * @file azure_blob_storage_uts.cpp
 * @brief Unit Tests for the Azure Blob Storage Utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <azure_blob_storage.h>
#include <azure_c_shared_utility/strings.h>
#include <catch2/catch.hpp>
#include <string>
#include <umock_c/umock_c.h>

class StringHandleWrapper
{
public:
    STRING_HANDLE extractedContainerName = nullptr;
    STRING_HANDLE expectedContainerName = nullptr;

    explicit StringHandleWrapper(const char* expectedName)
    {
        if (expectedName != nullptr)
        {
            expectedContainerName = STRING_construct(expectedName);

            if (expectedContainerName == nullptr)
            {
                throw std::runtime_error("expected container name could not be allocated");
            }
        }
    }
    StringHandleWrapper(const StringHandleWrapper&) = delete;
    StringHandleWrapper(StringHandleWrapper&&) = delete;
    StringHandleWrapper& operator=(const StringHandleWrapper&) = delete;
    StringHandleWrapper& operator=(StringHandleWrapper&&) = delete;

    ~StringHandleWrapper()
    {
        STRING_delete(extractedContainerName);
        STRING_delete(expectedContainerName);
    }
};

TEST_CASE("ParseContainerNameFromSasUrl Unit Tests")
{
    SECTION("Positive test- Properly formatted url")
    {
        std::string goodContainerUrl = "https://foo.bar.net/containerName";
        std::string expectedContainerName = "containerName";

        StringHandleWrapper testWrapper(expectedContainerName.c_str());

        CHECK(ParseContainerNameFromSasUrl(&testWrapper.extractedContainerName, goodContainerUrl.c_str()));
        CHECK(STRING_compare(testWrapper.expectedContainerName, testWrapper.extractedContainerName) == 0);
    }

    SECTION("Negative test- properly formatted sas url but has blob extension")
    {
        std::string blobExtensionUrl = "https://foo.bar.net/containerName/someblobtext.txt";

        StringHandleWrapper testWrapper(nullptr);

        CHECK_FALSE(ParseContainerNameFromSasUrl(&testWrapper.extractedContainerName, blobExtensionUrl.c_str()));
    }

    SECTION("Negative test- No container name")
    {
        std::string noContainerNameUrl = "https://foo.bar.net/";

        StringHandleWrapper testWrapper(nullptr);

        CHECK_FALSE(ParseContainerNameFromSasUrl(&testWrapper.extractedContainerName, noContainerNameUrl.c_str()));
    }

    SECTION("Negative test- imporper url")
    {
        std::string improperUrl = "foo.bar.net";

        StringHandleWrapper testWrapper(nullptr);

        CHECK_FALSE(ParseContainerNameFromSasUrl(&testWrapper.extractedContainerName, improperUrl.c_str()));
    }

    SECTION("Negative test- empty url")
    {
        StringHandleWrapper testWrapper(nullptr);

        CHECK_FALSE(ParseContainerNameFromSasUrl(&testWrapper.extractedContainerName, ""));
    }
}
