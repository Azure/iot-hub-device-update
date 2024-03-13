/**
 * @file jws_utils_ut.cpp
 * @brief Unit Tests for jws_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "base64_utils.h"
#include "crypto_lib.h"
#include "jws_utils.h"
#include <aduc/calloc_wrapper.hpp>
#include <aduc/result.h>
#include <aduc/rootkeypackage_utils.h>
#include <aduc/system_utils.h>
#include <azure_c_shared_utility/azure_base64.h>
#include <catch2/catch.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <regex>
#include <root_key_util.h>
#include <stdio.h>

#define ENABLE_MOCKS
#include "root_key_store.h"
#undef ENABLE_MOCKS

static std::string get_valid_example_rootkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/jws_utils/testrootkeypkg.json";
    return path;
};

std::string g_mockedRootKeyStorePath = "";

const char* MockRootKeyStore_GetRootKeyStorePath()
{
    return g_mockedRootKeyStorePath.c_str();
}

#if OPENSSL_VERSION_NUMBER < 0x1010000fL
#    define RSA_get0_n(x) ((x)->n)
#    define RSA_get0_e(x) ((x)->e)
#endif

bool CheckRSA_Key(
    CryptoKeyHandle key, BUFFER_HANDLE byteEBuff, const size_t e_len, BUFFER_HANDLE byteNBuff, const size_t N_len)
{
    bool success = false;
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    BIGNUM* key_N = nullptr;
    BIGNUM* key_e = nullptr;

    if (EVP_PKEY_get_bn_param(static_cast<EVP_PKEY*>(key), "n", &key_N) != 1)
    {
        return false;
    }

    if (EVP_PKEY_get_bn_param(static_cast<EVP_PKEY*>(key), "e", &key_e) != 1)
    {
        return false;
    }

#else
    const BIGNUM* key_N = nullptr;
    const BIGNUM* key_e = nullptr;
    RSA* rsa_key = EVP_PKEY_get0_RSA(static_cast<EVP_PKEY*>(key));

    if (rsa_key == nullptr)
    {
        return false;
    }

    RSA_get0_key(rsa_key, &key_N, &key_e, nullptr);

    if (key_N == nullptr || key_e == nullptr)
    {
        return false;
    }

#endif

    int key_N_size = BN_num_bytes(key_N);

    int key_e_size = BN_num_bytes(key_e);

    std::unique_ptr<uint8_t> key_N_bytes{ new uint8_t[key_N_size] };
    std::unique_ptr<uint8_t> key_e_bytes{ new uint8_t[key_e_size] };

    if (key_N_size != N_len)
    {
        return false;
    }

    if (key_e_size != e_len)
    {
        return false;
    }

    BN_bn2bin(key_N, key_N_bytes.get());
    BN_bn2bin(key_e, key_e_bytes.get());

    uint8_t* byteN = BUFFER_u_char(byteNBuff);
    uint8_t* byteE = BUFFER_u_char(byteEBuff);
    if (memcmp(key_N_bytes.get(), byteN, key_N_size) != 0)
    {
        goto done;
    }

    if (memcmp(key_e_bytes.get(), byteE, key_e_size) != 0)
    {
        goto done;
    }

    success = true;
done:
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    if (key_N != nullptr)
    {
        BN_free(key_N);
    }

    if (key_e != nullptr)
    {
        BN_free(key_e);
    }
#endif

    return success;
}

class TestCaseFixture
{
public:
    TestCaseFixture() : m_testPath{ ADUC_SystemUtils_GetTemporaryPathName() }
    {
        g_mockedRootKeyStorePath = get_valid_example_rootkey_package_json_path();

        ADUC_Result result = RootKeyUtility_ReloadPackageFromDisk(g_mockedRootKeyStorePath.c_str(), true);
        std::cout << g_mockedRootKeyStorePath << std::endl;
        CHECK(IsAducResultCodeSuccess(result.ResultCode));
    }

    ~TestCaseFixture()
    {
    }

    /**
     * @brief Gets the base test temp path as a c_str.
     *
     * @return const char* The test path.
     */
    const char* TestPath() const
    {
        return m_testPath.c_str();
    }

    /**
     * @brief provides the instance to this text fixture object for asserting
     * that the Context passed to the for-each callback matches the current
     * TestCaseFixture instance.
     *
     * @return TestCaseFixture* The pointer to this instance.
     */
    TestCaseFixture* Instance()
    {
        return this;
    }

private:
    TestCaseFixture(const TestCaseFixture&) = delete;
    TestCaseFixture& operator=(const TestCaseFixture&) = delete;
    TestCaseFixture(TestCaseFixture&&) = delete;
    TestCaseFixture& operator=(TestCaseFixture&&) = delete;

    std::string m_pkg{
        R"( {"protected":{"version":1,"published":1675972876,"disabledRootKeys":[],"disabledSigningKeys":[],"rootKeys":{"ADU.200702.R":{"keyType":"RSA","n":"1UIurxFUo1Blh6JNW7oa-6ky3-mZXwVFyK-9NR2J6CcnWKOo7sXFHk_3kqYSBn09fbAH9ix_3m0q9bxJvBXv8IHLP4hPJx2IcShgCLYZ0tI50AUfPHaGcbtZWLyxiHurVii_MXNEMhD9PdOWXP9OXLNr_4uEm4uAuEnQffrWQFh2TcByJ3XLmi-btJ8PJfEcxRsLWjB9L7jvpyZYU6_VHVUBUQ3pG6IPP9fpHSBBpuYUCq7-8hwq1uQEe_YUfuwPl4P6WPqBNiG5oyv62WELGpT3wb5_QBRKyfo1f-9mcACx_dvXYQ07WHRnlIl1dpZ8kYfSjhGX7nuHbJovRdhlP1JwmCrLyARj9clHz3D07WSndKUjj7bt9xzTsBxkVxJaqYGEH6DnUBmWtIKxrEjj4TKCy0AfrMRZvBA0UYL5KI2oHpv1eUV1styaEUMIvmHMmsTLdzb_g92ocU9Rjg57Tfp5mI2-_IJ-QEipEgGo2X7zpRvx-5B3PkCHGMmr2fd5","e":65537},"ADU.200703.R":{"keyType":"RSA","n":"sqOydBb6uyD5UnbmJz6AQcb-zzD5yJb1WQqqgedRg4rE9Rc6LyrmV9Rxzoo975pVdj6Z4sKuTO4tuHj1ok4o8pxOOWW87OQN5eM4qFmrCKQbtPSgUqM4s0YhE8w8aAbe_gCmkm7eTEcQ1hycJPXNcOH1anxoEx3hxfaoTyGfhnxExYqZHMXTBptacZ0JHMNkMWrFF5UdXSrxVcdm1Oj12albjKJsYmAFN9cysHPL90s2JyQhjDgKuBj-9RVgNYs17x4PiKYTjXt977PnsMmmHHB7zPIpi4f3vZ22iG-sc_9y8u9IJ5ZyhgaiXON9zrCe5cLZTsTzf3gHS2WIRQwR5ZZWNIgtFg5ZQtL32e0d7ck3d0R-44Q2n1gT72_kw0TUdwaKz1vIgByimGULNdxzyGnQXuglQ5722KsFr1EpI1VAWBDquOLNXXnM7N-0W5jH-uPSbCbOLixW4M-N7v2TEi8ASY0cgjhWpl15REoa89wWELPBLScR_huYBeSjYDGZ","e":65537}}},"signatures":[{"alg":"RS256","sig":"eW8Cn256fBmV0DfintpvKLKBJJ2estNVeBvriVcazxE0-R_eFfpA1lYFpaOTmVx1g8dcRFYCmCXnmqLcrEZFLRJ26GezCQxkMtgo5NhlzLAc5BhaWn4_HDx1Y1yObWvQf1ZYfMFIntEtCDYLK1DxmmtqFy-0uLBIC4vPXLCdW0g4sGlXskMt0caszgYSduHgAI6AicQqSGjAy6Sms3gWELR4xbSK765IDp4rWqXns_aLy8pbOgar4Uxusmz5ydmJ9p3epMIhthe1D_kNwhzg5egi5B_S3LgEbm5DiJwyewwNPdZH-xNzP4KhLUK0sZjXk21OE3pj5Ia-Eydkrm4K6puf_ZR1G_XwhLO8s0QKZnjYqIL_EldJdwcKnW6lZDOnkYYGb7NYYS8FxIP4AG8FannN0xD503fhd7bsyIQGaXEQwRZgV88oKQy_-EQFUZ2MvzAKq2Cg7_KoBFEfSmU5MZgPD-4OycU98bAtBVcK-3phFQPdtKPkqjaDqBF3pTK3"},{"alg":"RS256","sig":"Mj9AZXSqwu6NUWUvdLIbSMy--Yp68wWPOcsKSZ-9qToD0RIF7Q3rbgKCYC9FFzHzwBolBwsqZogHeEv0wGbj4EuCKRHrD1onc8AiBpUWD9QrySP8Ca3QzBeE1jDkGVJvmuYsviLzletYT-6GCEBBWuQyUSmbA0Az9x4sUg9BNF7M2_zyd4GGyHDSt9YVYJekv9IQwEinEUGW6wB8St_V3x4w1Pujl69azOI0VpTtXXTlw7xwyhq_gO4mCO40b8KBGdTdD1pHz_4UT4hHvoRl9nVRi4lKBCSEzpLr_Oqs2s7TwS13GEg-XMMkzd3jGVkFS9C9ezcJC8osaxg0i5z0g_lc785Rg1yXM-gytOYFn2xyWIzqvJ5CQn3XgkCO9lduYkEF78xHFNbsorup2c2GRZTdWpTwLEi0v6bv303CxhNMGJYiZull-lRVLANFVO_pewduE3DqDTs3PF2InX0m9_ve9XouDvooaw1q3Zk_BgNgcxQxSQv2ifP4EFrNvPg2"}]} )"
    };

    std::string m_testPath;
    ADUC_RootKeyPackage* m_rootKeyPackage;
};

TEST_CASE_METHOD(TestCaseFixture, "VerifySJWK")
{
    SECTION("Validate a Valid Signed JSON Web Key")
    {
        std::string signedJSONWebKey{ "eyJhbGciOiJSUzI1NiIsImtpZCI6IkFEVS4yMDA3MDIuUiJ9.eyJrdHkiOiJSU"
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
                                      "jIwMDcwMi5SLlMifQ.iSTgAEBXsd7AANkQMkaG-FAV6QOGUEuxuHg2YfSuWhtY"
                                      "XqbpM-jI5RVLKesSLCehK-lRC9x6-_LeyxNh1DOFc-Fa6oCEGwUj8ziOF_AT6s"
                                      "6EOmckqPrxuvCWtyYkkDRF74dtaK1jNA7SdXrZzvWCsMqOUMNz0gCoVR0Cs125"
                                      "4kFMRmRPVfEcjgT7j4lCpyDuWgr9SenSeqgKLYxjaaG0sRh9cdi2dKrwgaNaqA"
                                      "bHmCrrhxSPCTBzWMExZrLYzudEofyYHiVVRhSJpj0OQ18ecu4DPXV1Tct1y3k7"
                                      "LLio7n8izKuq2m3TxF9vPdqb9NP6Sc9-myaptpbFpHeFkUL-F5ytl_UBFKpwN9"
                                      "CL4wp6yZ-jdXNagrmU_qL1CyXw1omNCgTmJF3Gd3lyqKHHDerDs-MRpmKjwSwp"
                                      "ZCQJGDRcRovWyL12vjw3LBJMhmUxsEdBaZP5wGdsfD8ldKYFVFEcZ0orMNrUkS"
                                      "MAl6pIxtefEXiy5lqmiPzq_LJ1eRIrqY0_" };

        CHECK(VerifySJWK(signedJSONWebKey.c_str()) == JWSResult_Success);
    }

    SECTION("Attempt to Validate a an Signed JSON Web Key with a BadStructure")
    {
        // Note: JWK has been edited to only have two sections (only one period) instead of three
        std::string signedJSONWebKey{ "eyJhbGciOiJSUzI1NiIsImtpZCI6IkFEVS4yMDA3MDIuUiJ9eyJrdHkiOiJSU"
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
                                      "jIwMDcwMi5SLlMifQ.iSTgAEBXsd7AANkQMkaG-FAV6QOGUEuxuHg2YfSuWhtY"
                                      "XqbpM-jI5RVLKesSLCehK-lRC9x6-_LeyxNh1DOFc-Fa6oCEGwUj8ziOF_AT6s"
                                      "6EOmckqPrxuvCWtyYkkDRF74dtaK1jNA7SdXrZzvWCsMqOUMNz0gCoVR0Cs125"
                                      "4kFMRmRPVfEcjgT7j4lCpyDuWgr9SenSeqgKLYxjaaG0sRh9cdi2dKrwgaNaqA"
                                      "bHmCrrhxSPCTBzWMExZrLYzudEofyYHiVVRhSJpj0OQ18ecu4DPXV1Tct1y3k7"
                                      "LLio7n8izKuq2m3TxF9vPdqb9NP6Sc9-myaptpbFpHeFkUL-F5ytl_UBFKpwN9"
                                      "CL4wp6yZ-jdXNagrmU_qL1CyXw1omNCgTmJF3Gd3lyqKHHDerDs-MRpmKjwSwp"
                                      "ZCQJGDRcRovWyL12vjw3LBJMhmUxsEdBaZP5wGdsfD8ldKYFVFEcZ0orMNrUkS"
                                      "MAl6pIxtefEXiy5lqmiPzq_LJ1eRIrqY0_" };

        CHECK(VerifySJWK(signedJSONWebKey.c_str()) == JWSResult_BadStructure);
    }

    SECTION("Validating a Signed JSON Web Key with an invalid signature")
    {
        std::string signedJSONWebKey{ "eyJhbGciOiJSUzI1NiIsImtpZCI6IkFEVS4yMDA3MDIuUiJ9.eyJrdHkiOiJSU"
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
                                      "jIwMDcwMi5SLlMifQ.NOTgAEBXsd7AANkQMkaG-FAV6QOGUEuxuHg2YfSuWhtY"
                                      "XqbpM-jI5RVLKesSLCehK-lRC9x6-_LeyxNh1DOFc-Fa6oCEGwUj8ziOF_AT6s"
                                      "6EOmckqPrxuvCWtyYkkDRF74dtaK1jNA7SdXrZzvWCsMqOUMNz0gCoVR0Cs125"
                                      "4kFMRmRPVfEcjgT7j4lCpyDuWgr9SenSeqgKLYxjaaG0sRh9cdi2dKrwgaNaqA"
                                      "bHmCrrhxSPCTBzWMExZrLYzudEofyYHiVVRhSJpj0OQ18ecu4DPXV1Tct1y3k7"
                                      "LLio7n8izKuq2m3TxF9vPdqb9NP6Sc9-myaptpbFpHeFkUL-F5ytl_UBFKpwN9"
                                      "CL4wp6yZ-jdXNagrmU_qL1CyXw1omNCgTmJF3Gd3lyqKHHDerDs-MRpmKjwSwp"
                                      "ZCQJGDRcRovWyL12vjw3LBJMhmUxsEdBaZP5wGdsfD8ldKYFVFEcZ0orMNrUkS"
                                      "MAl6pIxtefEXiy5lqmiPzq_LJ1eRIrqY0_" };

        CHECK(VerifySJWK(signedJSONWebKey.c_str()) == JWSResult_InvalidSignature);
    }

    SECTION("Get Key From Signed JSON Web Key")
    {
        std::string signedJSONWebKey{ "eyJhbGciOiJSUzI1NiIsImtpZCI6IkFEVS4yMDA3MDIuUiJ9.eyJrdHkiOiJSU"
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
                                      "jIwMDcwMi5SLlMifQ.iSTgAEBXsd7AANkQMkaG-FAV6QOGUEuxuHg2YfSuWhtY"
                                      "XqbpM-jI5RVLKesSLCehK-lRC9x6-_LeyxNh1DOFc-Fa6oCEGwUj8ziOF_AT6s"
                                      "6EOmckqPrxuvCWtyYkkDRF74dtaK1jNA7SdXrZzvWCsMqOUMNz0gCoVR0Cs125"
                                      "4kFMRmRPVfEcjgT7j4lCpyDuWgr9SenSeqgKLYxjaaG0sRh9cdi2dKrwgaNaqA"
                                      "bHmCrrhxSPCTBzWMExZrLYzudEofyYHiVVRhSJpj0OQ18ecu4DPXV1Tct1y3k7"
                                      "LLio7n8izKuq2m3TxF9vPdqb9NP6Sc9-myaptpbFpHeFkUL-F5ytl_UBFKpwN9"
                                      "CL4wp6yZ-jdXNagrmU_qL1CyXw1omNCgTmJF3Gd3lyqKHHDerDs-MRpmKjwSwp"
                                      "ZCQJGDRcRovWyL12vjw3LBJMhmUxsEdBaZP5wGdsfD8ldKYFVFEcZ0orMNrUkS"
                                      "MAl6pIxtefEXiy5lqmiPzq_LJ1eRIrqY0_" };

        // Note: Known Key Values from the above Signed JSON Web Key
        std::string N{ "rHVBEFKR1vshg+AhIg/SDQO3x4kj3CUT7fGnJhAmuDhvHfj3gHzi0T2IAqC"
                       "1x2BCWdOo5v8tumqRj/nYpg97jjPCKucdO6m37dcOma46h7O0kHpwL3nUHG"
                       "ErJ5DA/apYnwEesexTjT8Sp/+bTqWEmzgD37pfdKaqjtHLGViYwVHPztBab"
                       "wwjhAvzyRY/y9OfmzDfXmrY1ro/+2hEqEykujwQEYkhjJa+B476+0muGyWI"
                       "5eIv/olt2ReXxMb9MllXNyoP3aNKIJibZMs7uKciwkyiUIaYcMjs9i/REy+"
                       "lM9vIZqrfpCUXu3tn1Kgc2Qs/Td8tNTCGV6wtVaqiIpTdT4RrCdMoO5SNef"
                       "fDyc2lC7u85+omTkcjPjm6fapdIyF2qemvSBDfB7caj5DII25Ww5EJcavfy"
                       "P54mqNQQ3GcMQb2dghiclpjYo+43ZgYCdGtaZd2EfLZwH3Qg2rDlfk/ia0/"
                       "1yqik/XZ1nsZTi0Bc5CpOMEqfNJFQk3BWoA05rCZ" };

        std::string e{ "AQAB" };

        BUFFER_HANDLE byteNBuff = Azure_Base64_Decode(N.c_str());
        size_t N_len = BUFFER_length(byteNBuff);

        CHECK(byteNBuff != nullptr);

        BUFFER_HANDLE byteEBuff = Azure_Base64_Decode(e.c_str());
        size_t e_len = BUFFER_length(byteEBuff);

        CHECK(byteEBuff != nullptr);

        CryptoKeyHandle key = GetKeyFromBase64EncodedJWK(signedJSONWebKey.c_str());

        // Check the key
        CHECK(CheckRSA_Key(key, byteEBuff, e_len, byteNBuff, N_len) == true);

        BUFFER_delete(byteNBuff);
        BUFFER_delete(byteEBuff);

        CryptoUtils_FreeCryptoKeyHandle(key);
    }
}

TEST_CASE_METHOD(TestCaseFixture, "VerifyJWSWithKey")
{
    SECTION("Validate a Valid JWS with a key")
    {
        // Note: Key parameters used to sign the JWT
        std::string N{ "rHVBEFKR1vshg+AhIg/SDQO3x4kj3CUT7fGnJhAmuDhvHfj3gHzi0T2IAqC"
                       "1x2BCWdOo5v8tumqRj/nYpg97jjPCKucdO6m37dcOma46h7O0kHpwL3nUHG"
                       "ErJ5DA/apYnwEesexTjT8Sp/+bTqWEmzgD37pfdKaqjtHLGViYwVHPztBab"
                       "wwjhAvzyRY/y9OfmzDfXmrY1ro/+2hEqEykujwQEYkhjJa+B476+0muGyWI"
                       "5eIv/olt2ReXxMb9MllXNyoP3aNKIJibZMs7uKciwkyiUIaYcMjs9i/REy+"
                       "lM9vIZqrfpCUXu3tn1Kgc2Qs/Td8tNTCGV6wtVaqiIpTdT4RrCdMoO5SNef"
                       "fDyc2lC7u85+omTkcjPjm6fapdIyF2qemvSBDfB7caj5DII25Ww5EJcavfy"
                       "P54mqNQQ3GcMQb2dghiclpjYo+43ZgYCdGtaZd2EfLZwH3Qg2rDlfk/ia0/"
                       "1yqik/XZ1nsZTi0Bc5CpOMEqfNJFQk3BWoA05rCZ" };

        std::string e{ "AQAB" };

        // JWT signed by the key above
        std::string signedJWT{ "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9"
                               "pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTU"
                               "RJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pY"
                               "2toV1FrVkdTMUl4ZG5Ob1p5dEJhRWxuTDFORVVVOHpl"
                               "RFJyYWpORFZWUTNaa2R1U21oQmJYVkVhSFpJWm1velo"
                               "waDZhVEJVTWtsQmNVTXhlREpDUTFka1QyODFkamgwZF"
                               "cxeFVtb3ZibGx3WnprM2FtcFFRMHQxWTJSUE5tMHpOM"
                               "lJqVDIxaE5EWm9OMDh3YTBod2Qwd3pibFZJUjBWeVNq"
                               "VkVRUzloY0ZsdWQwVmxjMlY0VkdwVU9GTndMeXRpVkh"
                               "GWFJXMTZaMFF6TjNCbVpFdGhjV3AwU0V4SFZtbFpkMV"
                               "pJVUhwMFFtRmlkM2RxYUVGMmVubFNXUzk1T1U5bWJYc"
                               "EVabGh0Y2xreGNtOHZLekpvUlhGRmVXdDFhbmRSUlZs"
                               "cmFHcEtZU3RDTkRjMkt6QnRkVWQ1VjBrMVpVbDJMMjl"
                               "zZERKU1pWaDRUV0k1VFd4c1dFNTViMUF6WVU1TFNVcH"
                               "BZbHBOY3pkMVMyTnBkMnQ1YVZWSllWbGpUV3B6T1drdl"
                               "VrVjVLMnhOT1haSlduRnlabkJEVlZoMU0zUnVNVXRuWX"
                               "pKUmN5OVVaRGgwVGxSRFIxWTJkM1JXWVhGcFNYQlVaRl"
                               "EwVW5KRFpFMXZUelZUVG1WbVprUjVZekpzUXpkMU9EVX"
                               "JiMjFVYTJOcVVHcHRObVpoY0dSSmVVWXljV1Z0ZGxOQ1"
                               "JHWkNOMk5oYWpWRVNVa3lOVmQzTlVWS1kyRjJabmxRTl"
                               "RSdGNVNVJVVE5IWTAxUllqSmtaMmhwWTJ4d2FsbHZLel"
                               "F6V21kWlEyUkhkR0ZhWkRKRlpreGFkMGd6VVdjeWNrUn"
                               "NabXN2YVdFd0x6RjVjV2xyTDFoYU1XNXpXbFJwTUVKak"
                               "5VTndUMDFGY1daT1NrWlJhek5DVjI5Qk1EVnlRMW9pTE"
                               "NKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaX"
                               "dpYTJsa0lqb2lRVVJWTGpJd01EY3dNaTVTTGxNaWZRLm"
                               "lTVGdBRUJYc2Q3QUFOa1FNa2FHLUZBVjZRT0dVRXV4dU"
                               "hnMllmU3VXaHRZWHFicE0takk1UlZMS2VzU0xDZWhLLW"
                               "xSQzl4Ni1fTGV5eE5oMURPRmMtRmE2b0NFR3dVajh6aU"
                               "9GX0FUNnM2RU9tY2txUHJ4dXZDV3R5WWtrRFJGNzRkdG"
                               "FLMWpOQTdTZFhyWnp2V0NzTXFPVU1OejBnQ29WUjBDcz"
                               "EyNTRrRk1SbVJQVmZFY2pnVDdqNGxDcHlEdVdncjlTZW"
                               "5TZXFnS0xZeGphYUcwc1JoOWNkaTJkS3J3Z2FOYXFBYk"
                               "htQ3JyaHhTUENUQnpXTUV4WnJMWXp1ZEVvZnlZSGlWVl"
                               "JoU0pwajBPUTE4ZWN1NERQWFYxVGN0MXkzazdMTGlvN2"
                               "44aXpLdXEybTNUeEY5dlBkcWI5TlA2U2M5LW15YXB0cG"
                               "JGcEhlRmtVTC1GNXl0bF9VQkZLcHdOOUNMNHdwNnlaLW"
                               "pkWE5hZ3JtVV9xTDFDeVh3MW9tTkNnVG1KRjNHZDNseX"
                               "FLSEhEZXJEcy1NUnBtS2p3U3dwWkNRSkdEUmNSb3ZXeU"
                               "wxMnZqdzNMQkpNaG1VeHNFZEJhWlA1d0dkc2ZEOGxkS1"
                               "lGVkZFY1owb3JNTnJVa1NNQWw2cEl4dGVmRVhpeTVscW"
                               "1pUHpxX0xKMWVSSXJxWTBfIn0.eyJzaGEyNTYiOiI3Mk"
                               "9BRTJmME5iVDArVEw5MzdvNzB4bzhvTzk2Z21WTFlESn"
                               "B4WEh6ZVhFPSJ9.Sagxe9ylLitBHD14QsqSCO1lhrsrq"
                               "qMdJo73at50-C3B2OVu6n5uiQ-6AOnuwEY07cRtxLcUl"
                               "i92HiLFy-itD57amI8ovIRuonLsJqcplmw6imdxDWD3C"
                               "CkV_I3LfUBqjuaBew71Q2HrddHn3KVTFp562xMYgFZmW"
                               "iERnz7c-q4IuH_7AqvNm8leznVrCscAs5UquHqz3oHLU"
                               "9xEn-Sur1aP0xlbN-USD9WET5wXLpiu9ECZ86CFTpc_i"
                               "3zlEKpl8Vbvsb0NHW_932Lrye6nz3TsYQNFxMcn5EIvH"
                               "ZoxIs_yHEtkJFyjFnktojrxFxGKZ5nFH-CrQH6VIwSSI"
                               "H1FkJOIJiI8QtovzlqdDkZNLMYQ3uM1yKt3anXTpwHbu"
                               "BrpYKQXN4T7bWN_9PWxyhnzKIDi6BulyrD8-H8X7P_S7"
                               "WBoFigb-nNrMFoSEm0qgAND01B0xJmsKf4Q6eB6L7k1S"
                               "0bJPx5DwrPVW-9TK8GXM0VjZYZGtiLCPUTa6SVRKTey" };

        // Build the key
        CryptoKeyHandle key = RSAKey_ObjFromB64Strings(N.c_str(), e.c_str());

        CHECK(VerifyJWSWithKey(signedJWT.c_str(), key) == JWSResult_Success);
        CryptoUtils_FreeCryptoKeyHandle(key);
    }

    SECTION("Validating an Invalid JWS")
    {
        // Note: Key parameters used to sign the JWT
        std::string N{ "rHVBEFKR1vshg+AhIg/SDQO3x4kj3CUT7fGnJhAmuDhvHfj3gHzi0T2IAqC"
                       "1x2BCWdOo5v8tumqRj/nYpg97jjPCKucdO6m37dcOma46h7O0kHpwL3nUHG"
                       "ErJ5DA/apYnwEesexTjT8Sp/+bTqWEmzgD37pfdKaqjtHLGViYwVHPztBab"
                       "wwjhAvzyRY/y9OfmzDfXmrY1ro/+2hEqEykujwQEYkhjJa+B476+0muGyWI"
                       "5eIv/olt2ReXxMb9MllXNyoP3aNKIJibZMs7uKciwkyiUIaYcMjs9i/REy+"
                       "lM9vIZqrfpCUXu3tn1Kgc2Qs/Td8tNTCGV6wtVaqiIpTdT4RrCdMoO5SNef"
                       "fDyc2lC7u85+omTkcjPjm6fapdIyF2qemvSBDfB7caj5DII25Ww5EJcavfy"
                       "P54mqNQQ3GcMQb2dghiclpjYo+43ZgYCdGtaZd2EfLZwH3Qg2rDlfk/ia0/"
                       "1yqik/XZ1nsZTi0Bc5CpOMEqfNJFQk3BWoA05rCZ" };

        std::string e{ "AQAB" };

        // JWT signed by the key above
        std::string signedJWT{ "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9"
                               "pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTU"
                               "RJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pY"
                               "2toV1FrVkdTMUl4ZG5Ob1p5dEJhRWxuTDFORVVVOHpl"
                               "RFJyYWpORFZWUTNaa2R1U21oQmJYVkVhSFpJWm1velo"
                               "waDZhVEJVTWtsQmNVTXhlREpDUTFka1QyODFkamgwZF"
                               "cxeFVtb3ZibGx3WnprM2FtcFFRMHQxWTJSUE5tMHpOM"
                               "lJqVDIxaE5EWm9OMDh3YTBod2Qwd3pibFZJUjBWeVNq"
                               "VkVRUzloY0ZsdWQwVmxjMlY0VkdwVU9GTndMeXRpVkh"
                               "GWFJXMTZaMFF6TjNCbVpFdGhjV3AwU0V4SFZtbFpkMV"
                               "pJVUhwMFFtRmlkM2RxYUVGMmVubFNXUzk1T1U5bWJYc"
                               "EVabGh0Y2xreGNtOHZLekpvUlhGRmVXdDFhbmRSUlZs"
                               "cmFHcEtZU3RDTkRjMkt6QnRkVWQ1VjBrMVpVbDJMMjl"
                               "zZERKU1pWaDRUV0k1VFd4c1dFNTViMUF6WVU1TFNVcH"
                               "BZbHBOY3pkMVMyTnBkMnQ1YVZWSllWbGpUV3B6T1drdl"
                               "VrVjVLMnhOT1haSlduRnlabkJEVlZoMU0zUnVNVXRuWX"
                               "pKUmN5OVVaRGgwVGxSRFIxWTJkM1JXWVhGcFNYQlVaRl"
                               "EwVW5KRFpFMXZUelZUVG1WbVprUjVZekpzUXpkMU9EVX"
                               "JiMjFVYTJOcVVHcHRObVpoY0dSSmVVWXljV1Z0ZGxOQ1"
                               "JHWkNOMk5oYWpWRVNVa3lOVmQzTlVWS1kyRjJabmxRTl"
                               "RSdGNVNVJVVE5IWTAxUllqSmtaMmhwWTJ4d2FsbHZLel"
                               "F6V21kWlEyUkhkR0ZhWkRKRlpreGFkMGd6VVdjeWNrUn"
                               "NabXN2YVdFd0x6RjVjV2xyTDFoYU1XNXpXbFJwTUVKak"
                               "5VTndUMDFGY1daT1NrWlJhek5DVjI5Qk1EVnlRMW9pTE"
                               "NKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaX"
                               "dpYTJsa0lqb2lRVVJWTGpJd01EY3dNaTVTTGxNaWZRLm"
                               "lTVGdBRUJYc2Q3QUFOa1FNa2FHLUZBVjZRT0dVRXV4dU"
                               "hnMllmU3VXaHRZWHFicE0takk1UlZMS2VzU0xDZWhLLW"
                               "xSQzl4Ni1fTGV5eE5oMURPRmMtRmE2b0NFR3dVajh6aU"
                               "9GX0FUNnM2RU9tY2txUHJ4dXZDV3R5WWtrRFJGNzRkdG"
                               "FLMWpOQTdTZFhyWnp2V0NzTXFPVU1OejBnQ29WUjBDcz"
                               "EyNTRrRk1SbVJQVmZFY2pnVDdqNGxDcHlEdVdncjlTZW"
                               "5TZXFnS0xZeGphYUcwc1JoOWNkaTJkS3J3Z2FOYXFBYk"
                               "htQ3JyaHhTUENUQnpXTUV4WnJMWXp1ZEVvZnlZSGlWVl"
                               "JoU0pwajBPUTE4ZWN1NERQWFYxVGN0MXkzazdMTGlvN2"
                               "44aXpLdXEybTNUeEY5dlBkcWI5TlA2U2M5LW15YXB0cG"
                               "JGcEhlRmtVTC1GNXl0bF9VQkZLcHdOOUNMNHdwNnlaLW"
                               "pkWE5hZ3JtVV9xTDFDeVh3MW9tTkNnVG1KRjNHZDNseX"
                               "FLSEhEZXJEcy1NUnBtS2p3U3dwWkNRSkdEUmNSb3ZXeU"
                               "wxMnZqdzNMQkpNaG1VeHNFZEJhWlA1d0dkc2ZEOGxkS1"
                               "lGVkZFY1owb3JNTnJVa1NNQWw2cEl4dGVmRVhpeTVscW"
                               "1pUHpxX0xKMWVSSXJxWTBfIn0.eyJzaGEyNTYiOiI3Mk"
                               "9BRTJmME5iVDArVEw5MzdvNzB4bzhvTzk2Z21WTFlESn"
                               "B4WEh6ZVhFPSJ9.asdxe9ylLitBHD14QsqSCO1lhrsrq"
                               "qMdJo73at50-C3B2OVu6n5uiQ-6AOnuwEY07cRtxLcUl"
                               "i92HiLFy-itD57amI8ovIRuonLsJqcplmw6imdxDWD3C"
                               "CkV_I3LfUBqjuaBew71Q2HrddHn3KVTFp562xMYgFZmW"
                               "iERnz7c-q4IuH_7AqvNm8leznVrCscAs5UquHqz3oHLU"
                               "9xEn-Sur1aP0xlbN-USD9WET5wXLpiu9ECZ86CFTpc_i"
                               "3zlEKpl8Vbvsb0NHW_932Lrye6nz3TsYQNFxMcn5EIvH"
                               "ZoxIs_yHEtkJFyjFnktojrxFxGKZ5nFH-CrQH6VIwSSI"
                               "H1FkJOIJiI8QtovzlqdDkZNLMYQ3uM1yKt3anXTpwHbu"
                               "BrpYKQXN4T7bWN_9PWxyhnzKIDi6BulyrD8-H8X7P_S7"
                               "WBoFigb-nNrMFoSEm0qgAND01B0xJmsKf4Q6eB6L7k1S"
                               "0bJPx5DwrPVW-9TK8GXM0VjZYZGtiLCPUTa6SVRKTey" };

        // Build the key
        CryptoKeyHandle key = RSAKey_ObjFromB64Strings(N.c_str(), e.c_str());

        CHECK(VerifyJWSWithKey(signedJWT.c_str(), key) == JWSResult_InvalidSignature);

        CryptoUtils_FreeCryptoKeyHandle(key);
    }

    SECTION("Getting the Payload from a JWT")
    {
        // Note: Key parameters used to sign the JWT
        std::string N{ "rHVBEFKR1vshg+AhIg/SDQO3x4kj3CUT7fGnJhAmuDhvHfj3gHzi0T2IAqC"
                       "1x2BCWdOo5v8tumqRj/nYpg97jjPCKucdO6m37dcOma46h7O0kHpwL3nUHG"
                       "ErJ5DA/apYnwEesexTjT8Sp/+bTqWEmzgD37pfdKaqjtHLGViYwVHPztBab"
                       "wwjhAvzyRY/y9OfmzDfXmrY1ro/+2hEqEykujwQEYkhjJa+B476+0muGyWI"
                       "5eIv/olt2ReXxMb9MllXNyoP3aNKIJibZMs7uKciwkyiUIaYcMjs9i/REy+"
                       "lM9vIZqrfpCUXu3tn1Kgc2Qs/Td8tNTCGV6wtVaqiIpTdT4RrCdMoO5SNef"
                       "fDyc2lC7u85+omTkcjPjm6fapdIyF2qemvSBDfB7caj5DII25Ww5EJcavfy"
                       "P54mqNQQ3GcMQb2dghiclpjYo+43ZgYCdGtaZd2EfLZwH3Qg2rDlfk/ia0/"
                       "1yqik/XZ1nsZTi0Bc5CpOMEqfNJFQk3BWoA05rCZ" };

        std::string e{ "AQAB" };

        // JWT signed by the key above
        std::string signedJWT{ "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9"
                               "pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTU"
                               "RJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pY"
                               "2toV1FrVkdTMUl4ZG5Ob1p5dEJhRWxuTDFORVVVOHpl"
                               "RFJyYWpORFZWUTNaa2R1U21oQmJYVkVhSFpJWm1velo"
                               "waDZhVEJVTWtsQmNVTXhlREpDUTFka1QyODFkamgwZF"
                               "cxeFVtb3ZibGx3WnprM2FtcFFRMHQxWTJSUE5tMHpOM"
                               "lJqVDIxaE5EWm9OMDh3YTBod2Qwd3pibFZJUjBWeVNq"
                               "VkVRUzloY0ZsdWQwVmxjMlY0VkdwVU9GTndMeXRpVkh"
                               "GWFJXMTZaMFF6TjNCbVpFdGhjV3AwU0V4SFZtbFpkMV"
                               "pJVUhwMFFtRmlkM2RxYUVGMmVubFNXUzk1T1U5bWJYc"
                               "EVabGh0Y2xreGNtOHZLekpvUlhGRmVXdDFhbmRSUlZs"
                               "cmFHcEtZU3RDTkRjMkt6QnRkVWQ1VjBrMVpVbDJMMjl"
                               "zZERKU1pWaDRUV0k1VFd4c1dFNTViMUF6WVU1TFNVcH"
                               "BZbHBOY3pkMVMyTnBkMnQ1YVZWSllWbGpUV3B6T1drdl"
                               "VrVjVLMnhOT1haSlduRnlabkJEVlZoMU0zUnVNVXRuWX"
                               "pKUmN5OVVaRGgwVGxSRFIxWTJkM1JXWVhGcFNYQlVaRl"
                               "EwVW5KRFpFMXZUelZUVG1WbVprUjVZekpzUXpkMU9EVX"
                               "JiMjFVYTJOcVVHcHRObVpoY0dSSmVVWXljV1Z0ZGxOQ1"
                               "JHWkNOMk5oYWpWRVNVa3lOVmQzTlVWS1kyRjJabmxRTl"
                               "RSdGNVNVJVVE5IWTAxUllqSmtaMmhwWTJ4d2FsbHZLel"
                               "F6V21kWlEyUkhkR0ZhWkRKRlpreGFkMGd6VVdjeWNrUn"
                               "NabXN2YVdFd0x6RjVjV2xyTDFoYU1XNXpXbFJwTUVKak"
                               "5VTndUMDFGY1daT1NrWlJhek5DVjI5Qk1EVnlRMW9pTE"
                               "NKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaX"
                               "dpYTJsa0lqb2lRVVJWTGpJd01EY3dNaTVTTGxNaWZRLm"
                               "lTVGdBRUJYc2Q3QUFOa1FNa2FHLUZBVjZRT0dVRXV4dU"
                               "hnMllmU3VXaHRZWHFicE0takk1UlZMS2VzU0xDZWhLLW"
                               "xSQzl4Ni1fTGV5eE5oMURPRmMtRmE2b0NFR3dVajh6aU"
                               "9GX0FUNnM2RU9tY2txUHJ4dXZDV3R5WWtrRFJGNzRkdG"
                               "FLMWpOQTdTZFhyWnp2V0NzTXFPVU1OejBnQ29WUjBDcz"
                               "EyNTRrRk1SbVJQVmZFY2pnVDdqNGxDcHlEdVdncjlTZW"
                               "5TZXFnS0xZeGphYUcwc1JoOWNkaTJkS3J3Z2FOYXFBYk"
                               "htQ3JyaHhTUENUQnpXTUV4WnJMWXp1ZEVvZnlZSGlWVl"
                               "JoU0pwajBPUTE4ZWN1NERQWFYxVGN0MXkzazdMTGlvN2"
                               "44aXpLdXEybTNUeEY5dlBkcWI5TlA2U2M5LW15YXB0cG"
                               "JGcEhlRmtVTC1GNXl0bF9VQkZLcHdOOUNMNHdwNnlaLW"
                               "pkWE5hZ3JtVV9xTDFDeVh3MW9tTkNnVG1KRjNHZDNseX"
                               "FLSEhEZXJEcy1NUnBtS2p3U3dwWkNRSkdEUmNSb3ZXeU"
                               "wxMnZqdzNMQkpNaG1VeHNFZEJhWlA1d0dkc2ZEOGxkS1"
                               "lGVkZFY1owb3JNTnJVa1NNQWw2cEl4dGVmRVhpeTVscW"
                               "1pUHpxX0xKMWVSSXJxWTBfIn0.eyJzaGEyNTYiOiI3Mk"
                               "9BRTJmME5iVDArVEw5MzdvNzB4bzhvTzk2Z21WTFlESn"
                               "B4WEh6ZVhFPSJ9.asdxe9ylLitBHD14QsqSCO1lhrsrq"
                               "qMdJo73at50-C3B2OVu6n5uiQ-6AOnuwEY07cRtxLcUl"
                               "i92HiLFy-itD57amI8ovIRuonLsJqcplmw6imdxDWD3C"
                               "CkV_I3LfUBqjuaBew71Q2HrddHn3KVTFp562xMYgFZmW"
                               "iERnz7c-q4IuH_7AqvNm8leznVrCscAs5UquHqz3oHLU"
                               "9xEn-Sur1aP0xlbN-USD9WET5wXLpiu9ECZ86CFTpc_i"
                               "3zlEKpl8Vbvsb0NHW_932Lrye6nz3TsYQNFxMcn5EIvH"
                               "ZoxIs_yHEtkJFyjFnktojrxFxGKZ5nFH-CrQH6VIwSSI"
                               "H1FkJOIJiI8QtovzlqdDkZNLMYQ3uM1yKt3anXTpwHbu"
                               "BrpYKQXN4T7bWN_9PWxyhnzKIDi6BulyrD8-H8X7P_S7"
                               "WBoFigb-nNrMFoSEm0qgAND01B0xJmsKf4Q6eB6L7k1S"
                               "0bJPx5DwrPVW-9TK8GXM0VjZYZGtiLCPUTa6SVRKTey" };

        std::string expectedEncodedpayload{ "eyJzaGEyNTYiOiI3Mk9BRTJmME5iVDA"
                                            "rVEw5MzdvNzB4bzhvTzk2Z21WTFlESn"
                                            "B4WEh6ZVhFPSJ9" };

        char* expectedDecodedPayloadString = Base64URLDecodeToString(expectedEncodedpayload.c_str());

        CHECK(expectedDecodedPayloadString != nullptr);

        // Build the key
        CryptoKeyHandle key = RSAKey_ObjFromB64Strings(N.c_str(), e.c_str());

        ADUC::StringUtils::cstr_wrapper payload;
        CHECK(GetPayloadFromJWT(signedJWT.c_str(), payload.address_of()) == JWSResult_Success);
        CHECK(payload.get() != nullptr);

        size_t payload_len = strlen(payload.get());
        size_t expected_payload_len = strlen(expectedDecodedPayloadString);
        CHECK(payload_len == expected_payload_len);

        CHECK(strcmp(payload.get(), expectedDecodedPayloadString) == 0);
        CryptoUtils_FreeCryptoKeyHandle(key);
    }
}

const char* AllowedSigningKey =
    "ucKAJMkskVVKjtVLFdraMSd0cTa2Vcndkle540smg3a2v4hYXoHWBaA0tkZj5VM1fWR-XcjHJ9NRh74TzsHqPJODXn085tWGMwOzUEPhOSAzRaY-FCr23SIqM6AHCYPxziKbz9kEcD6e043UyCRMyLf8fQJ3SOvBXCNoVSkiQ8rwcDeHjFiSzk_BLy0JGRjfzJZF8l-q1N-Vqpq3VtOmQJphblSL6bC9AR1GNrvaJbHiSciaFvuiucneVBu3B6bY0wEin20x_CrjTNmiWEtuY_zoUxJGLGQVHkzJRRAQweHxw_FDSMd3UhiINRuN7Qb3r_S9HPoFNkZvaOUVOVe7WUY0jAFIzVUEcq2CTx43p0XvaLeYEz-DsG-RlPkkT2i-1ykEhwtJfsKGDTIP5mPDslZkTUScgZFRMToJdwOtGKkAzGXQPlvtf3IL49fUTM4r8dpIc7E1N2Djt94__kcdY1e8JxfgRH7RoiQCATHep6-mQW5UKq_onJW2bNo7i9Gb";
const char* DisallowedSigningKey =
    "2WisuSoVDzKsz02BmP2bulWJzwrDH4hBIgKaz4Ithol_LYOSpk0knonvCiEB5Zb9kKMUAlOdKluvO2nGKp95kqZzm77thqjUbe5bZyFOCqPlPH-0nUHhg_oHXvX_5Oz3l-7KhMG0bUWzQ72nkmUHViexAPBpY-u4zZTRr8MONbGtMInrVI7SJTbToZ1zzM-b04o8wxlNfNJspjY2P_82mmJXZKlRlGdWuLYLoeXhKosfSu9MP1aLjC-puEmYmZ-dsoMg3_DHhluC-7VN2r8dAm3e3cTKuL3bNvGguTwTnccMEw1VxLMUsnpsDtxFHjebwpRVvs76JJsW3fllYZZ2T1l5WWxQbWDCdui7dDvnAfEww7Juxw4dKdXnlSorBGa5-QxZ0OnfKQTYbQweA_GehkKPPvku9mzK-n0PxfsaQMLS1-JfXgiVNQ4erhu_625iKFwWKlfuqOuZWiJMkFTK-NBmpDKAaBtxdH_5Xgc3ZA7SMymyVfw-9UmWv-ooK1F9";

static CONSTBUFFER_HANDLE GetHashPubKey(const char* n)
{
    CONSTBUFFER_HANDLE pubkey = CryptoUtils_GenerateRsaPublicKey(n, "AQAB");
    REQUIRE(pubkey != nullptr);

    CONSTBUFFER_HANDLE hashed = CryptoUtils_CreateSha256Hash(pubkey);
    CONSTBUFFER_DecRef(pubkey);
    REQUIRE(hashed != nullptr);

    return hashed;
}

static VECTOR_HANDLE GetSigningKeyDisallowedList(const char* signing_key_N)
{
    VECTOR_HANDLE disallowedSigningPubKeyHashes = VECTOR_create(sizeof(ADUC_RootKeyPackage_Hash));
    REQUIRE(disallowedSigningPubKeyHashes != nullptr);

    if (signing_key_N != nullptr)
    {
        CONSTBUFFER_HANDLE hashPubKeyHandle = GetHashPubKey(signing_key_N);
        REQUIRE(hashPubKeyHandle != nullptr);

        ADUC_RootKeyPackage_Hash rootkeypkg_hash = {
            SHA256, /* alg */
            hashPubKeyHandle /* hash */
        };

        VECTOR_push_back(disallowedSigningPubKeyHashes, &rootkeypkg_hash, 1);
    }

    return disallowedSigningPubKeyHashes;
}

static std::string GetSJWKJson(const char* signing_key_N)
{
    const std::string sjwk_template{ R"( {                        )"
                                     R"( "kty": "RSA",            )"
                                     R"( "alg": "RS256",          )"
                                     R"( "kid": "ADU.210609.R.S", )"
                                     R"( "n": "<%MODULUS%>",      )"
                                     R"( "e": "AQAB"              )"
                                     R"( }                        )" };
    return std::regex_replace(sjwk_template, std::regex("<%MODULUS%>"), std::string{ signing_key_N });
}

TEST_CASE("IsSigningKeyDisallowed")
{
    SECTION("Missing - Empty list")
    {
        VECTOR_HANDLE disallowedSigningKeys = GetSigningKeyDisallowedList(nullptr); // empty vector

        std::string sjwk{ GetSJWKJson(AllowedSigningKey) }; // SJWK has allowed signing key
        JWSResult jwsResult = IsSigningKeyDisallowed(sjwk.c_str(), disallowedSigningKeys);

        CHECK(jwsResult == JWSResult_Success); // Namely, it is not JWSResult_DisallowedSigningKey.

        VECTOR_destroy(disallowedSigningKeys);
    }

    SECTION("Missing - Non-empty list")
    {
        VECTOR_HANDLE disallowedSigningKeys = GetSigningKeyDisallowedList(DisallowedSigningKey);

        std::string sjwk{ GetSJWKJson(AllowedSigningKey) };
        JWSResult jwsResult = IsSigningKeyDisallowed(sjwk.c_str(), disallowedSigningKeys);

        CHECK(jwsResult == JWSResult_Success); // Allowed is not Disallowed that is in the disallow list.

        VECTOR_destroy(disallowedSigningKeys);
    }

    SECTION("sjwk key found in Disallow list")
    {
        VECTOR_HANDLE disallowedSigningKeys = GetSigningKeyDisallowedList(DisallowedSigningKey);

        std::string sjwk{ GetSJWKJson(DisallowedSigningKey) };
        JWSResult jwsResult = IsSigningKeyDisallowed(sjwk.c_str(), disallowedSigningKeys);

        CHECK(jwsResult == JWSResult_DisallowedSigningKey); // Disallowed signing key is enforced.

        VECTOR_destroy(disallowedSigningKeys);
    }
}
