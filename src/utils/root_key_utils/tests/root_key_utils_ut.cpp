/**
 * @file crypto_utils_ut.cpp
 * @brief Unit Tests for c_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "base64_utils.h"
#include "crypto_lib.h"
#include <aduc/calloc_wrapper.hpp>
#include <catch2/catch.hpp>
#include <cstring>

using ADUC::StringUtils::cstr_wrapper;
using uint8_t_wrapper = ADUC::StringUtils::calloc_wrapper<uint8_t>;


TEST_CASE("RSA Keys")
{
    SECTION("Making an RSA Key From a String")
    {
    }

    SECTION("Getting a Root Key ID")
    {
    }

    SECTION("Failing to get a Root Key")
    {
    }
}
TEST_CASE("RootKeyUtil_ValidateRootKeyPackage Signature Validation")
{
    SECTION("Validating a Valid Signature")
    {

    }

    SECTION("Validating an Invalid Signature")
    {
    }
}
