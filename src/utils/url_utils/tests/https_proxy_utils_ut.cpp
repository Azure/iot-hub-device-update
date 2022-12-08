/**
 * @file https_proxy_utils_ut.cpp
 * @brief Unit Tests for https_proxy_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/https_proxy_utils.h"
#include <catch2/catch.hpp>

using Catch::Matchers::Equals;

class TestCaseFixture
{
public:
    TestCaseFixture()
    {
        existing_https_proxy = getenv("https_proxy");
        existing_HTTPS_PROXY = getenv("HTTPS_PROXY");
        unsetenv("https_proxy");
        unsetenv("HTTPS_PROXY");
    }

    ~TestCaseFixture()
    {
        if (existing_https_proxy != nullptr)
        {
            setenv("https_proxy", existing_https_proxy, 1);
        }
        else
        {
            unsetenv("https_proxy");
        }

        if (existing_HTTPS_PROXY != nullptr)
        {
            setenv("HTTPS_PROXY", existing_HTTPS_PROXY, 1);
        }
        else
        {
            unsetenv("HTTPS_PROXY");
        }
    }

    TestCaseFixture(const TestCaseFixture&) = delete;
    TestCaseFixture& operator=(const TestCaseFixture&) = delete;
    TestCaseFixture(TestCaseFixture&&) = delete;
    TestCaseFixture& operator=(TestCaseFixture&&) = delete;

private:
    char* existing_https_proxy;
    char* existing_HTTPS_PROXY;
};

TEST_CASE_METHOD(TestCaseFixture, "Parse https_proxy (escaped)")
{
    setenv("https_proxy", "http%3A%2F%2F100.0.0.1%3A8888", 1);

    HTTP_PROXY_OPTIONS proxyOptions = {};
    _Bool proxyOk = InitializeProxyOptions(&proxyOptions);
    CHECK(proxyOk);
    CHECK_THAT(proxyOptions.host_address, Equals("100.0.0.1"));
    CHECK(proxyOptions.port == 8888);
    CHECK(proxyOptions.username == nullptr);
    CHECK(proxyOptions.password == nullptr);
    UninitializeProxyOptions(&proxyOptions);
}

TEST_CASE_METHOD(TestCaseFixture, "Parse https_proxy")
{
    setenv("https_proxy", "http://100.0.0.1:8888", 1);

    HTTP_PROXY_OPTIONS proxyOptions = {};
    _Bool proxyOk = InitializeProxyOptions(&proxyOptions);
    CHECK(proxyOk);
    CHECK_THAT(proxyOptions.host_address, Equals("100.0.0.1"));
    CHECK(proxyOptions.port == 8888);
    CHECK(proxyOptions.username == nullptr);
    CHECK(proxyOptions.password == nullptr);
    UninitializeProxyOptions(&proxyOptions);
}

TEST_CASE_METHOD(TestCaseFixture, "Parse HTTPS_PROXY (uppper case)")
{
    setenv("HTTPS_PROXY", "http://222.0.0.1:123", 1);

    HTTP_PROXY_OPTIONS proxyOptions = {};
    _Bool proxyOk = InitializeProxyOptions(&proxyOptions);
    CHECK(proxyOk);
    CHECK_THAT(proxyOptions.host_address, Equals("222.0.0.1"));
    CHECK(proxyOptions.port == 123);
    CHECK(proxyOptions.username == nullptr);
    CHECK(proxyOptions.password == nullptr);
    UninitializeProxyOptions(&proxyOptions);
}

// If both https_proxy and HTTPS_PROXY exist, use https_proxy.
TEST_CASE_METHOD(TestCaseFixture, "Use https_proxy (lower case)")
{
    setenv("https_proxy", "http://100.0.0.1:8888", 1);
    setenv("HTTPS_PROXY", "http://222.0.0.1:123", 1);

    HTTP_PROXY_OPTIONS proxyOptions = {};
    _Bool proxyOk = InitializeProxyOptions(&proxyOptions);
    CHECK(proxyOk);
    CHECK_THAT(proxyOptions.host_address, Equals("100.0.0.1"));
    CHECK(proxyOptions.port == 8888);
    CHECK(proxyOptions.username == nullptr);
    CHECK(proxyOptions.password == nullptr);
    UninitializeProxyOptions(&proxyOptions);
}

TEST_CASE_METHOD(TestCaseFixture, "Parse username and password")
{
    setenv("https_proxy", "http://username:password@100.0.0.1:8888", 1);

    HTTP_PROXY_OPTIONS proxyOptions = {};
    _Bool proxyOk = InitializeProxyOptions(&proxyOptions);
    CHECK(proxyOk);
    CHECK_THAT(proxyOptions.host_address, Equals("100.0.0.1"));
    CHECK(proxyOptions.port == 8888);
    CHECK_THAT(proxyOptions.username, Equals("username"));
    CHECK_THAT(proxyOptions.password, Equals("password"));
    UninitializeProxyOptions(&proxyOptions);
}

TEST_CASE_METHOD(TestCaseFixture, "No port number")
{
    setenv("https_proxy", "http://username:password@100.0.0.1", 1);

    HTTP_PROXY_OPTIONS proxyOptions = {};
    _Bool proxyOk = InitializeProxyOptions(&proxyOptions);
    CHECK(proxyOk);
    CHECK_THAT(proxyOptions.host_address, Equals("100.0.0.1"));
    CHECK(proxyOptions.port == 0);
    CHECK_THAT(proxyOptions.username, Equals("username"));
    CHECK_THAT(proxyOptions.password, Equals("password"));
    UninitializeProxyOptions(&proxyOptions);
}

TEST_CASE_METHOD(TestCaseFixture, "Empty username")
{
    setenv("https_proxy", "http://:password@100.0.0.1", 1);

    HTTP_PROXY_OPTIONS proxyOptions = {};
    _Bool proxyOk = InitializeProxyOptions(&proxyOptions);
    CHECK(proxyOk);
    CHECK_THAT(proxyOptions.host_address, Equals("100.0.0.1"));
    CHECK(proxyOptions.port == 0);
    CHECK(proxyOptions.username == nullptr);
    CHECK_THAT(proxyOptions.password, Equals("password"));
    UninitializeProxyOptions(&proxyOptions);
}

TEST_CASE_METHOD(TestCaseFixture, "Empty password (supported)")
{
    setenv("https_proxy", "http://username:@100.0.0.1:8888", 1);

    HTTP_PROXY_OPTIONS proxyOptions = {};
    _Bool proxyOk = InitializeProxyOptions(&proxyOptions);
    CHECK(proxyOk);
    CHECK_THAT(proxyOptions.host_address, Equals("100.0.0.1"));
    CHECK(proxyOptions.port == 8888);
    CHECK_THAT(proxyOptions.username, Equals("username"));
    CHECK(proxyOptions.password == nullptr);
    UninitializeProxyOptions(&proxyOptions);
}
