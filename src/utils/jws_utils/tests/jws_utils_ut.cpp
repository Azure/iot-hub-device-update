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
#include <azure_c_shared_utility/azure_base64.h>
#include <catch2/catch.hpp>
#include <cstring>
#include <openssl/evp.h>
#include <openssl/rsa.h>

using ADUC::StringUtils::cstr_wrapper;

#if OPENSSL_VERSION_NUMBER < 0x1010000fL
#    define RSA_get0_n(x) ((x)->n)
#    define RSA_get0_e(x) ((x)->e)
#endif

TEST_CASE("VerifySJWK")
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
        RSA* rsa_key = EVP_PKEY_get0_RSA(static_cast<EVP_PKEY*>(key));

        CHECK(rsa_key != nullptr);

        const BIGNUM* key_N = nullptr;
        const BIGNUM* key_e = nullptr;

        RSA_get0_key(rsa_key,&key_N,&key_e,nullptr);

        int key_N_size = BN_num_bytes(key_N);
        int key_e_size = BN_num_bytes(key_e);

        CHECK(key_N_size == N_len);
        CHECK(key_e_size == e_len);

        std::unique_ptr<uint8_t> key_N_bytes{ new uint8_t[key_N_size] };
        std::unique_ptr<uint8_t> key_e_bytes{ new uint8_t[key_e_size] };
        BN_bn2bin(key_N, key_N_bytes.get());
        BN_bn2bin(key_e, key_e_bytes.get());

        uint8_t* byteN = BUFFER_u_char(byteNBuff);
        uint8_t* byteE = BUFFER_u_char(byteEBuff);
        CHECK(memcmp(key_N_bytes.get(), byteN, key_N_size) == 0);
        CHECK(memcmp(key_e_bytes.get(), byteE, key_e_size) == 0);

        BUFFER_delete(byteNBuff);
        BUFFER_delete(byteEBuff);

        FreeCryptoKeyHandle(key);
    }
}

TEST_CASE("VerifyJWSWithKey")
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
        FreeCryptoKeyHandle(key);
    }

    SECTION("Validating an InValid JWS")
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

        FreeCryptoKeyHandle(key);
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

        cstr_wrapper payload;
        CHECK(GetPayloadFromJWT(signedJWT.c_str(), payload.address_of()) == JWSResult_Success);
        CHECK(payload.get() != nullptr);

        size_t payload_len = strlen(payload.get());
        size_t expected_payload_len = strlen(expectedDecodedPayloadString);
        CHECK(payload_len == expected_payload_len);

        CHECK(strcmp(payload.get(), expectedDecodedPayloadString) == 0);
        FreeCryptoKeyHandle(key);
    }
}
