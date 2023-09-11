/**
 * @file string_utils_ut.cpp
 * @brief Unit Tests for c_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch.hpp>
#include <vector>
using Catch::Matchers::Equals;

#include "aduc/crypto_test_utils.hpp"

TEST_CASE("KeyPair")
{
    SECTION("Basic API")
    {
        aduc::testutils::KeyPair key_pair{ 2048 };
        try
        {
            key_pair.Generate();

            CHECK(key_pair.GetHashOfPublicKey().length() > 0);
            CHECK(key_pair.GetModulus().length() > 0);
            CHECK(key_pair.GetExponent() == 0x10001);

            auto signature = key_pair.SignData(std::string{"foo"});
            CHECK(signature.length() > 0);

            CHECK(key_pair.VerifySignature(signature));
        } catch (...) {
            CHECK(false);
        }
    }
}

