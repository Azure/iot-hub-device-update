/**
 * @file crypto_utils_ut.cpp
 * @brief Unit Tests for c_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "base64_utils.h"
#include "crypto_lib.h"
#include "decryption_alg_types.h"
#include <aduc/calloc_wrapper.hpp>
#include <catch2/catch.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
using ADUC::StringUtils::cstr_wrapper;
using uint8_t_wrapper = ADUC::StringUtils::calloc_wrapper<uint8_t>;
using keydata_wrapper = ADUC::StringUtils::calloc_wrapper<KeyData>;

TEST_CASE("Base64 Encoding")
{
    SECTION("Encoding a Base64 URL ")
    {
        const std::array<uint8_t, 16> test_bytes{ '|', '|', '|', '|', '\\', '\\', '\\', '/',
                                                  '/', '/', '/', '?', '}',  '}',  '~',  '~' };

        const char* expected_output = "fHx8fFxcXC8vLy8_fX1-fg";

        cstr_wrapper output{ Base64URLEncode(test_bytes.data(), test_bytes.size()) };

        CHECK(output.get() != nullptr);

        CHECK(strcmp(expected_output, output.get()) == 0);
    }
}

TEST_CASE("Base64 Decoding")
{
    SECTION("Decoding in Base64 URL")
    {
        const std::array<uint8_t, 16> expected_output{ '|', '|', '|', '|', '\\', '\\', '\\', '/',
                                                       '/', '/', '/', '?', '}',  '}',  '~',  '~' };
        const std::string test_input = "fHx8fFxcXC8vLy8_fX1-fg==";

        uint8_t_wrapper output_handle;
        size_t out_len = Base64URLDecode(test_input.c_str(), output_handle.address_of());

        CHECK(out_len == expected_output.size());
        CHECK(output_handle.get() != nullptr);

        CHECK(memcmp(output_handle.get(), expected_output.data(), expected_output.size()) == 0);
    }
}

TEST_CASE("RSA Keys")
{
    SECTION("Making an RSA Key From a String")
    {
        // Use a known N and e string
        const char* N =
            "l5snRXKtZkLbo4e9lGPn6UjbMYaTgDcr/NHaruvnanbL1IugKtqby8g+KT1ynsAC4UaayQpPPuFMP4JqQGrtyu78QjJmCNu0olIntxyeccyM+GrI0Z22Sr19/19DGpIjObXBAZs1IBrEylMl2D5opk/qbanl550sxPew1Ze//Jeb9SwNJHF4iT3l7HDcj8SrmMv1uKX55Uknsp265jo8HWBXppbJ+aQP63jKGbRBuJvgYI48oejxFIMcNdUpjsLgqCYD/Edn7pfAVPC+BlOQzj7J8mLuCcXp+wPint/nj6q7FylAR2QHEmisi47MsaFiawLA80xsL2oHylla7b0EpQ==";
        const char* e = "AQAB";

        CryptoKeyHandle key = RSAKey_ObjFromB64Strings(N, e);

        CHECK(key != nullptr);

        FreeCryptoKeyHandle(key);
    }

    SECTION("Getting a Root Key ID")
    {
        CryptoKeyHandle key = GetRootKeyForKeyID("ADU.200702.R");

        CHECK(key != nullptr);

        FreeCryptoKeyHandle(key);
    }

    SECTION("Failing to get a Root Key")
    {
        CryptoKeyHandle key = GetRootKeyForKeyID("foo");
        CHECK(key == nullptr);
    }
}

TEST_CASE("Signature Verification")
{
    SECTION("Validating a Valid Signature")
    {
        std::string signature{ "iSTgAEBXsd7AANkQMkaG-FAV6QOGUEuxuHg2YfSuWhtY"
                               "XqbpM-jI5RVLKesSLCehK-lRC9x6-_LeyxNh1DOFc-Fa6oCEGwUj8ziOF_AT6s"
                               "6EOmckqPrxuvCWtyYkkDRF74dtaK1jNA7SdXrZzvWCsMqOUMNz0gCoVR0Cs125"
                               "4kFMRmRPVfEcjgT7j4lCpyDuWgr9SenSeqgKLYxjaaG0sRh9cdi2dKrwgaNaqA"
                               "bHmCrrhxSPCTBzWMExZrLYzudEofyYHiVVRhSJpj0OQ18ecu4DPXV1Tct1y3k7"
                               "LLio7n8izKuq2m3TxF9vPdqb9NP6Sc9-myaptpbFpHeFkUL-F5ytl_UBFKpwN9"
                               "CL4wp6yZ-jdXNagrmU_qL1CyXw1omNCgTmJF3Gd3lyqKHHDerDs-MRpmKjwSwp"
                               "ZCQJGDRcRovWyL12vjw3LBJMhmUxsEdBaZP5wGdsfD8ldKYFVFEcZ0orMNrUkS"
                               "MAl6pIxtefEXiy5lqmiPzq_LJ1eRIrqY0_" };

        std::string blob{ "eyJhbGciOiJSUzI1NiIsImtpZCI6IkFEVS4yMDA3MDIuUiJ9.eyJrdHkiOiJSU"
                          "0EiLCJuIjoickhWQkVGS1IxdnNoZytBaElnL1NEUU8zeDRrajNDVVQ3ZkduSmh"
                          "BbXVEaHZIZmozZ0h6aTBUMklBcUMxeDJCQ1dkT281djh0dW1xUmovbllwZzk3a"
                          "mpQQ0t1Y2RPNm0zN2RjT21hNDZoN08wa0hwd0wzblVIR0VySjVEQS9hcFlud0V"
                          "lc2V4VGpUOFNwLytiVHFXRW16Z0QzN3BmZEthcWp0SExHVmlZd1ZIUHp0QmFid"
                          "3dqaEF2enlSWS95OU9mbXpEZlhtclkxcm8vKzJoRXFFeWt1andRRVlraGpKYSt"
                          "CNDc2KzBtdUd5V0k1ZUl2L29sdDJSZVh4TWI5TWxsWE55b1AzYU5LSUppYlpNc"
                          "zd1S2Npd2t5aVVJYVljTWpzOWkvUkV5K2xNOXZJWnFyZnBDVVh1M3RuMUtnYzJ"
                          "Rcy9UZDh0TlRDR1Y2d3RWYXFpSXBUZFQ0UnJDZE1vTzVTTmVmZkR5YzJsQzd1O"
                          "DUrb21Ua2NqUGptNmZhcGRJeUYycWVtdlNCRGZCN2NhajVESUkyNVd3NUVKY2F"
                          "2ZnlQNTRtcU5RUTNHY01RYjJkZ2hpY2xwallvKzQzWmdZQ2RHdGFaZDJFZkxad"
                          "0gzUWcyckRsZmsvaWEwLzF5cWlrL1haMW5zWlRpMEJjNUNwT01FcWZOSkZRazN"
                          "CV29BMDVyQ1oiLCJlIjoiQVFBQiIsImFsZyI6IlJTMjU2Iiwia2lkIjoiQURVL"
                          "jIwMDcwMi5SLlMifQ" };

        CryptoKeyHandle key = GetRootKeyForKeyID("ADU.200702.R");

        uint8_t* d_sig_handle = nullptr;
        size_t sig_len = Base64URLDecode(signature.c_str(), &d_sig_handle);

        CHECK(IsValidSignature(
            "RS256",
            d_sig_handle,
            sig_len,
            reinterpret_cast<const uint8_t*>(blob.c_str()), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            blob.length(),
            key));
    }

    SECTION("Validating an Invalid Signature")
    {
        // Note: Signature has been garbled to create an invalid signature
        std::string signature{ "asdgAEBXsd7AANkQMkaG-FAV6QOGUEuxuHg2YfSuWhtY"
                               "XqbpM-jI5RVLKesSLCehK-lRC9x6-_LeyxNh1DOFc-Fa6oCEGwUj8ziOF_AT6s"
                               "6EOmckqPrxuvCWtyYkkDRF74dtaK1jNA7SdXrZzvWCsMqOUMNz0gCoVR0Cs125"
                               "4kFMRmRPVfEcjgT7j4lCpyDuWgr9SenSeqgKLYxjaaG0sRh9cdi2dKrwgaNaqA"
                               "bHmCrrhxSPCTBzWMExZrLYzudEofyYHiVVRhSJpj0OQ18ecu4DPXV1Tct1y3k7"
                               "LLio7n8izKuq2m3TxF9vPdqb9NP6Sc9-myaptpbFpHeFkUL-F5ytl_UBFKpwN9"
                               "CL4wp6yZ-jdXNagrmU_qL1CyXw1omNCgTmJF3Gd3lyqKHHDerDs-MRpmKjwSwp"
                               "ZCQJGDRcRovWyL12vjw3LBJMhmUxsEdBaZP5wGdsfD8ldKYFVFEcZ0orMNrUkS"
                               "MAl6pIxtefEXiy5lqmiPzq_LJ1eRIrqY0_" };

        std::string blob{ "eyJhbGciOiJSUzI1NiIsImtpZCI6IkFEVS4yMDA3MDIuUiJ9.eyJrdHkiOiJSU"
                          "0EiLCJuIjoickhWQkVGS1IxdnNoZytBaElnL1NEUU8zeDRrajNDVVQ3ZkduSmh"
                          "BbXVEaHZIZmozZ0h6aTBUMklBcUMxeDJCQ1dkT281djh0dW1xUmovbllwZzk3a"
                          "mpQQ0t1Y2RPNm0zN2RjT21hNDZoN08wa0hwd0wzblVIR0VySjVEQS9hcFlud0V"
                          "lc2V4VGpUOFNwLytiVHFXRW16Z0QzN3BmZEthcWp0SExHVmlZd1ZIUHp0QmFid"
                          "3dqaEF2enlSWS95OU9mbXpEZlhtclkxcm8vKzJoRXFFeWt1andRRVlraGpKYSt"
                          "CNDc2KzBtdUd5V0k1ZUl2L29sdDJSZVh4TWI5TWxsWE55b1AzYU5LSUppYlpNc"
                          "zd1S2Npd2t5aVVJYVljTWpzOWkvUkV5K2xNOXZJWnFyZnBDVVh1M3RuMUtnYzJ"
                          "Rcy9UZDh0TlRDR1Y2d3RWYXFpSXBUZFQ0UnJDZE1vTzVTTmVmZkR5YzJsQzd1O"
                          "DUrb21Ua2NqUGptNmZhcGRJeUYycWVtdlNCRGZCN2NhajVESUkyNVd3NUVKY2F"
                          "2ZnlQNTRtcU5RUTNHY01RYjJkZ2hpY2xwallvKzQzWmdZQ2RHdGFaZDJFZkxad"
                          "0gzUWcyckRsZmsvaWEwLzF5cWlrL1haMW5zWlRpMEJjNUNwT01FcWZOSkZRazN"
                          "CV29BMDVyQ1oiLCJlIjoiQVFBQiIsImFsZyI6IlJTMjU2Iiwia2lkIjoiQURVL"
                          "jIwMDcwMi5SLlMifQ" };

        CryptoKeyHandle key = GetRootKeyForKeyID("ADU.200702.R");

        uint8_t* d_sig_handle = nullptr;
        size_t sig_len = Base64URLDecode(signature.c_str(), &d_sig_handle);

        CHECK(!IsValidSignature(
            "RS256",
            d_sig_handle,
            sig_len,
            reinterpret_cast<const uint8_t*>(blob.c_str()), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            blob.length(),
            key));
    }
}

TEST_CASE("CryptoUtils_InitAndAllocKeyDataFromUrlEncodedB64String")
{
    SECTION("Successful Init")
    {
        std::string b64EncodedKeyData = "urq6usAMCRI0VnjL2voQAg==";

        std::array<uint8_t, 16> decodedBytes = { 0xBA, 0xBA, 0xBA, 0xBA, 0xC0, 0x0C, 0x09, 0x12,
                                                 0x34, 0x56, 0x78, 0xCB, 0xDA, 0xFA, 0x10, 0x02 };

        keydata_wrapper key;

        REQUIRE(CryptoUtils_InitAndAllocKeyDataFromUrlEncodedB64String(key.address_of(), b64EncodedKeyData.c_str()));

        CHECK(key.get() != nullptr);

        REQUIRE(key.get()->keyData != nullptr);

        const CONSTBUFFER* keyBytes = CONSTBUFFER_GetContent(key.get()->keyData);

        REQUIRE(keyBytes != nullptr);

        CHECK(keyBytes->size == decodedBytes.size());

        for (int i = 0; i < keyBytes->size; ++i)
        {
            CHECK(keyBytes->buffer[i] == decodedBytes.at(i));
        }

        CryptoUtils_DeAllocKeyData(key.get());
    }

    SECTION("Bad Init")
    {
        KeyData* key = nullptr;

        CHECK_FALSE(CryptoUtils_InitAndAllocKeyDataFromUrlEncodedB64String(&key, nullptr));

        CHECK(key == nullptr);
    }
}

TEST_CASE("CryptoUtils_IsKeyNullOrEmpty")
{
    SECTION("Check- Non Empty")
    {
        std::string b64EncodedKeyData = "urq6usAMCRI0VnjL2vo=";

        keydata_wrapper key;

        REQUIRE(CryptoUtils_InitAndAllocKeyDataFromUrlEncodedB64String(key.address_of(), b64EncodedKeyData.c_str()));

        REQUIRE(key.get() != nullptr);

        CHECK_FALSE(CryptoUtils_IsKeyNullOrEmpty(key.get()));

        CryptoUtils_DeAllocKeyData(key.get());
    }

    SECTION("Check - Null KeyData")
    {
        keydata_wrapper key;

        CHECK(CryptoUtils_IsKeyNullOrEmpty(key.get()));
    }
}

TEST_CASE("CryptoUtils_EncryptBufferBlockByBlock")
{
    SECTION("Encrypt a Buffer")
    {
        std::string b64UrlEncodedKeyData = "BIltO-l5mkKpPGANKtX0nCaYAo90gARHo7M8y_RSx2s";

        std::string content = "MicrosoftDevsRule!\n";

        std::string expectedEncodedOutput = "oAHfa0HqwjPO3_25WKhKbUWE0PLuXc1P8CiCPyOpcMw";

        keydata_wrapper key;

        REQUIRE(
            CryptoUtils_InitAndAllocKeyDataFromUrlEncodedB64String(key.address_of(), b64UrlEncodedKeyData.c_str()));

        BUFFER_HANDLE encryptedContent = nullptr;

        CONSTBUFFER_HANDLE contentBuffer = CONSTBUFFER_Create((unsigned char*)content.c_str(), content.length());

        ADUC_Result result =
            CryptoUtils_EncryptBufferBlockByBlock(&encryptedContent, AES_256_CBC, key.get(), contentBuffer);

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        CHECK(encryptedContent != nullptr);
        CHECK(BUFFER_length(encryptedContent) != 0);

        char* b64UrlEncodedEncryptedContent =
            Base64URLEncode(BUFFER_u_char(encryptedContent), BUFFER_length(encryptedContent));

        cstr_wrapper encodedEncryptedContent{ b64UrlEncodedEncryptedContent };

        std::string encodedEncryptedContentStr{ encodedEncryptedContent.get() };

        CHECK(encodedEncryptedContentStr == expectedEncodedOutput);

        BUFFER_delete(encryptedContent);
        CryptoUtils_DeAllocKeyData(key.get());
        CONSTBUFFER_DecRef(contentBuffer);
    }
    SECTION("Decrypt a Buffer")
    {
        std::string b64UrlEncodedKeyData = "BIltO-l5mkKpPGANKtX0nCaYAo90gARHo7M8y_RSx2s";

        std::string expectedOutput = "MicrosoftDevsRule!\n";

        std::string encryptedContent = "oAHfa0HqwjPO3_25WKhKbUWE0PLuXc1P8CiCPyOpcMw";

        keydata_wrapper key;

        char* decodedContent = nullptr;

        REQUIRE(
            CryptoUtils_InitAndAllocKeyDataFromUrlEncodedB64String(key.address_of(), b64UrlEncodedKeyData.c_str()));

        uint8_t_wrapper decodedContent_w;
        size_t decodedContentLength = Base64URLDecode(encryptedContent.c_str(), decodedContent_w.address_of());

        REQUIRE(decodedContent_w.get() != nullptr);
        REQUIRE(decodedContentLength == 32);

        BUFFER_HANDLE decryptedContent = NULL;
        CONSTBUFFER_HANDLE encryptedContentBuffer = CONSTBUFFER_Create(decodedContent_w.get(), decodedContentLength);

        ADUC_Result result =
            CryptoUtils_DecryptBufferBlockByBlock(&decryptedContent, AES_256_CBC, key.get(), encryptedContentBuffer);

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        CHECK(decryptedContent != nullptr);

        const size_t decryptedContentLength = BUFFER_length(decryptedContent);

        CHECK(decryptedContentLength == expectedOutput.length());

        //
        // turn the decrypted content into a string
        //
        uint8_t* decryptedContentBytes = BUFFER_u_char(decryptedContent);

        std::string decryptedContentStr{ decryptedContentBytes, decryptedContentBytes + decryptedContentLength };

        CHECK(decryptedContentStr == expectedOutput);

        BUFFER_delete(decryptedContent);
        CryptoUtils_DeAllocKeyData(key.get());
        CONSTBUFFER_DecRef(encryptedContentBuffer);
    }
}

TEST_CASE("CryptoUtils_DecryptBufferBlockByBlock")
{
    SECTION("Decrypt a valid Buffer")
    {
        std::string b64UrlEncodedKeyData = "BIltO-l5mkKpPGANKtX0nCaYAo90gARHo7M8y_RSx2s";

        std::string expectedOutput = "MicrosoftDevsRule!\n";

        std::string encryptedContent = "oAHfa0HqwjPO3_25WKhKbUWE0PLuXc1P8CiCPyOpcMw";

        keydata_wrapper key;

        char* decodedContent = nullptr;

        REQUIRE(
            CryptoUtils_InitAndAllocKeyDataFromUrlEncodedB64String(key.address_of(), b64UrlEncodedKeyData.c_str()));

        uint8_t_wrapper decodedContent_w;
        size_t decodedContentLength = Base64URLDecode(encryptedContent.c_str(), decodedContent_w.address_of());

        REQUIRE(decodedContent_w.get() != nullptr);
        REQUIRE(decodedContentLength == 32);

        BUFFER_HANDLE decryptedContent = NULL;
        CONSTBUFFER_HANDLE encryptedContentBuffer = CONSTBUFFER_Create(decodedContent_w.get(), decodedContentLength);

        ADUC_Result result =
            CryptoUtils_DecryptBufferBlockByBlock(&decryptedContent, AES_256_CBC, key.get(), encryptedContentBuffer);

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        const size_t decryptedContentLength = BUFFER_length(decryptedContent);

        uint8_t* decryptedContentBytes = BUFFER_u_char(decryptedContent);

        CHECK(decryptedContentBytes != nullptr);
        CHECK(decryptedContentLength != 0);

        std::string decryptedContentStr{ decryptedContentBytes, decryptedContentBytes + decryptedContentLength };
        CHECK(decryptedContentStr == expectedOutput);

        BUFFER_delete(decryptedContent);
        CryptoUtils_DeAllocKeyData(key.get());
        CONSTBUFFER_DecRef(encryptedContentBuffer);
    }
}
