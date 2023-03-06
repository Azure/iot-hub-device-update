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
#include <iostream>

using ADUC::StringUtils::cstr_wrapper;
using uint8_t_wrapper = ADUC::StringUtils::calloc_wrapper<uint8_t>;

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

TEST_CASE("CryptoUtils_InitializeKeyDataFromB64String")
{
    SECTION("Successful Init")
    {
        std::string b64EncodedKeyData = "urq6usAMCRI0VnjL2voQAg==";

        std::array<uint8_t, 16> decodedBytes = { 0xBA, 0xBA, 0xBA, 0xBA, 0xC0, 0x0C, 0x09, 0x12,
                                                 0x34, 0x56, 0x78, 0xCB, 0xDA, 0xFA, 0x10, 0x02 };

        KeyData* key = nullptr;

        REQUIRE(CryptoUtils_InitializeKeyDataFromB64String(&key, b64EncodedKeyData.c_str()));

        CHECK(key != nullptr);

        REQUIRE(key->keyData != nullptr);

        const CONSTBUFFER* keyBytes = CONSTBUFFER_GetContent(key->keyData);

        REQUIRE(keyBytes != nullptr);

        CHECK(keyBytes->size == decodedBytes.size());

        for (int i = 0; i < keyBytes->size; ++i)
        {
            CHECK(keyBytes->buffer[i] == decodedBytes.at(i));
        }

        CryptoUtils_DeInitializeKeyData(&key);
    }

    SECTION("Bad Init")
    {
        KeyData* key = nullptr;

        CHECK_FALSE(CryptoUtils_InitializeKeyDataFromB64String(&key, nullptr));

        CHECK(key == nullptr);
    }
}

TEST_CASE("CryptoUtils_IsKeyNullOrEmpty")
{
    SECTION("Check- Non Empty")
    {
        std::string b64EncodedKeyData = "urq6usAMCRI0VnjL2vo=";

        KeyData* key = nullptr;

        REQUIRE(CryptoUtils_InitializeKeyDataFromB64String(&key, b64EncodedKeyData.c_str()));

        REQUIRE(key != nullptr);

        CHECK_FALSE(CryptoUtils_IsKeyNullOrEmpty(key));

        CryptoUtils_DeInitializeKeyData(&key);
    }

    SECTION("Check - Null KeyData")
    {
        KeyData* key = nullptr;

        CHECK(CryptoUtils_IsKeyNullOrEmpty(key));
    }
}

TEST_CASE("CryptoUtils_EncryptBufferBlockByBlock")
{
    SECTION("Encrypt One Block to a Buffer")
    {
        //
        // Script for generating content: openssl enc -aes-128-cbc -in ./test-content.txt -out ./test-content.txt.enc -p -K BABABABAC00C0912345678CBDAFA1002 -iv 00000000000000000000000000000000 -v -nosalt -a
        // Website to help with shit: https://cryptii.com/pipes/hex-to-base64
        //
        std::string content = "abcdefghijklmno\n";
        std::string b64EncodedKeyData = "urq6usAMCRI0VnjL2voQAg==";

        KeyData* key = nullptr;

        REQUIRE(CryptoUtils_InitializeKeyDataFromB64String(&key, b64EncodedKeyData.c_str()));

        cstr_wrapper encryptedContent;
        size_t encryptedOutSize = 0;

        ADUC_Result result = CryptoUtils_EncryptBufferBlockByBlock(
            (unsigned char**)encryptedContent.address_of(),
            &encryptedOutSize,
            AES_128_CBC,
            key,
            content.c_str(),
            content.length());

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        CHECK(encryptedContent.get() != nullptr);
        CHECK(encryptedOutSize != 0);

        cstr_wrapper encodedOutput { Base64URLEncode((uint8_t*)encryptedContent.get(),outputSize) };

        CHECK(encodedOutput.get() != nullptr);

        std::cout << encodedOutput.get() << std::endl;
        // std::string encodedOutputStr {encodedOutput.get()};

        // CHECK(encodedOutputStr == expectedB64EncodedOutput)

        cstr_wrapper decryptedContent;
        size_t decryptedOutSize = 0;

        result = CryptoUtils_DecryptBufferBlockByBlock(
            (unsigned char**)decryptedContent.address_of(),
            &decryptedOutSize,
            AES_128_CBC,
            key,
            (const unsigned char*)encryptedContent.get(),
            encryptedOutSize);

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        CHECK(decryptedContent.get() != nullptr);
        CHECK(decryptedOutSize != 0);

        std::string decryptedContentStr{ decryptedContent.get() };

        CHECK(decryptedContentStr == content);

        CryptoUtils_DeInitializeKeyData(&key);
    }
    SECTION("Encrypt Multiple Even Blocks to a Buffer")
    {
        std::string content =
            "AQuickBrownFoxJumpedOverTheFenceAndStoleATortoisesShellToSellBecauseFoxesAreFrustratedWithTheCurrentStateOfPublicTransport.Yes.";
    }
    SECTION("Encrypt Multiple Blocks that Needs Padding")
    {
        std::string content =
            "AQuickBrownFoxJumpedOverTheFenceAndStoleATortoisesShellToSellBecauseFoxesAreFrustratedWithTheCurrentStateOfPublicTransp";
    }
}

TEST_CASE("CryptoUtils_DecryptBufferBlockByBlock")
{
    SECTION("Decrypt Content to a Buffer")
    {
    }
}
