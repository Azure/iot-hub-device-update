/**
 * @file eis_utils_ut.cpp
 * @brief Unit Tests for eis_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "eis_utils.h"
#include "umock_c/umock_c.h"
#include <aduc/adu_types.h>
#include <aduc/c_utils.h>
#include <aduc/calloc_wrapper.hpp>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <catch2/catch.hpp>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string.h>
#include <time.h>

#define ENABLE_MOCKS
#include "eis_coms.h"
#undef ENABLE_MOCKS

using ADUC::StringUtils::cstr_wrapper;

//
// UMock Enum Type Definitions
//

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, readability-non-const-parameter, cppcoreguidelines-pro-bounds-constant-array-index): clang-tidy can't handle the way that UMock expands macro's so we're suppressing
IMPLEMENT_UMOCK_C_ENUM_TYPE(
    EISErr,
    EISErr_Failed,
    EISErr_Ok,
    EISErr_InvalidArg,
    EISErr_ConnErr,
    EISErr_TimeoutErr,
    EISErr_HTTPErr,
    EISErr_RecvInvalidValueErr,
    EISErr_RecvRespOutOfLimitsErr,
    EISErr_ContentAllocErr)

//
// Response Strings
//

// clang-format off
static const char* validDeviceSasIdentityResponseStr =
    R"({)"
        R"("type":"aziot",)"
        R"("spec":{)"
            R"("hubName":"foo.example-devices.net",)"
            R"("deviceId":"user-test-device",)"
            R"("auth":{)"
                R"("type":"sas",)"
                R"("keyHandle":"primary")"
            R"(})"
        R"(})"
    R"(})";

static const char* validDeviceX509IdentityResponseStr =
    R"({)"
        R"("type":"aziot",)"
        R"("spec":{)"
            R"("hubName":"foo.example-devices.net",)"
            R"("deviceId":"user-test-device",)"
            R"("auth":{)"
                R"("type":"x509",)"
                R"("keyHandle":"primary",)"
                R"("certId":"foo-cert")"
            R"(})"
        R"(})"
    R"(})";

static const char* validModuleSasIdentityResponseStr =
    R"({)"
        R"("type":"aziot",)"
        R"("spec":{)"
            R"("hubName":"foo.example-devices.net",)"
            R"("deviceId":"user-test-device",)"
            R"("moduleId":"user-test-module",)"
            R"("auth":{)"
                R"("type":"sas",)"
                R"("keyHandle":"primary")"
            R"(})"
        R"(})"
    R"(})";

static const char* validModuleX509IdentityResponseStr =
    R"({)"
        R"("type":"aziot",)"
        R"("spec":{)"
            R"("hubName":"foo.example-devices.net",)"
            R"("deviceId":"user-test-device",)"
            R"("moduleId":"user-test-module",)"
            R"("auth":{)"
                R"("type":"x509",)"
                R"("keyHandle":"primary",)"
                R"("certId":"foo-cert")"
            R"(})"
        R"(})"
    R"(})";

static const char* validSignatureResponseStr =
    R"({)"
        R"("signature":"hIuFfERqcDBnu84EwVlF01JfiaRvH6A20dMWQW6T4fg=")"
    R"(})";

static const char* validCertificateResponseStr =
    R"({)"
        R"("pem":"HikdasdfasdWErfrasdaxcasdfasdf....ASDFASDFAWefsdafdv")"
    R"(})";

static const char* invalidCertificateResponseStr = R"({})";

static const char* invalidIdentityResponseStr =
    R"({)"
        R"("type":"aziot",)"
        R"("spec":{)"
        R"(})"
    R"(})";

static const char* invalidSignatureResponseStr = "{}";
// clang-format on

//
// Global Mocks for Requests to the EIS service
//
static char* g_identityResp = nullptr;
static char* g_signatureResp = nullptr;
static char* g_certificateResp = nullptr;

static EISErr MockHookRequestIdentitiesFromEIS(unsigned int timeoutMS, char** responseBuffer)
{
    UNREFERENCED_PARAMETER(timeoutMS);

    *responseBuffer = g_identityResp;
    return EISErr_Ok;
}

static EISErr MockHookRequestSignatureFromEIS(
    const char* keyHandle, const char* deviceUri, const char* expiry, unsigned int timeoutMS, char** responseBuffer)
{
    UNREFERENCED_PARAMETER(keyHandle);
    UNREFERENCED_PARAMETER(deviceUri);
    UNREFERENCED_PARAMETER(expiry);
    UNREFERENCED_PARAMETER(timeoutMS);

    *responseBuffer = g_signatureResp;
    return EISErr_Ok;
}

static EISErr MockHookRequestCertificateFromEIS(const char* certId, unsigned int timeoutMS, char** responseBuffer)
{
    UNREFERENCED_PARAMETER(certId);
    UNREFERENCED_PARAMETER(timeoutMS);

    *responseBuffer = g_certificateResp;
    return EISErr_Ok;
}

class GlobalMockHookTestCaseFixture
{
public:
    GlobalMockHookTestCaseFixture()
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast): clang-tidy can't handle the way that UMock expands macro's so we're suppressing
        REGISTER_TYPE(EISErr, EISErr);

        REGISTER_GLOBAL_MOCK_HOOK(RequestIdentitiesFromEIS, MockHookRequestIdentitiesFromEIS);
        REGISTER_GLOBAL_MOCK_HOOK(RequestSignatureFromEIS, MockHookRequestSignatureFromEIS);
        REGISTER_GLOBAL_MOCK_HOOK(RequestCertificateFromEIS, MockHookRequestCertificateFromEIS);
    }

    ~GlobalMockHookTestCaseFixture() = default;

    GlobalMockHookTestCaseFixture(const GlobalMockHookTestCaseFixture&) = delete;
    GlobalMockHookTestCaseFixture& operator=(const GlobalMockHookTestCaseFixture&) = delete;
    GlobalMockHookTestCaseFixture(GlobalMockHookTestCaseFixture&&) = delete;
    GlobalMockHookTestCaseFixture& operator=(GlobalMockHookTestCaseFixture&&) = delete;
};

TEST_CASE_METHOD(GlobalMockHookTestCaseFixture, "RequestConnectionStringFromEISWithExpiry Functional Tests")
{
    SECTION("RequestConnectionStringFromEISWithExpiry DeviceId , SAS Token Success Test")
    {
        const auto expiry = static_cast<time_t>(time(nullptr) + 86400); // Expiry is one day after the unit test is run
        uint32_t timeout = 5000;

        // Dynamically build the connectionString with the proper value
        std::stringstream expectedConnStrStream;
        expectedConnStrStream << "HostName=foo.example-devices.net;"
                              << "DeviceId=user-test-device;"
                              << "SharedAccessSignature=SharedAccessSignature "
                              << "sr=foo.example-devices.net/devices/user-test-device&"
                              << "sig=hIuFfERqcDBnu84EwVlF01JfiaRvH6A20dMWQW6T4fg%3d&"
                              << "se=" << expiry;

        // Note: These do not need to be freed! They are returned by the RequestSignature/IdentityFromEIS
        // functions. They are freed within the code block
        REQUIRE(mallocAndStrcpy_s(&g_identityResp, validDeviceSasIdentityResponseStr) == 0);
        REQUIRE(mallocAndStrcpy_s(&g_signatureResp, validSignatureResponseStr) == 0);

        ADUC_ConnectionInfo outInfo = {
            ADUC_AuthType_NotSet, ADUC_ConnType_NotSet, nullptr, nullptr, nullptr, nullptr
        };

        EISUtilityResult result = RequestConnectionStringFromEISWithExpiry(expiry, timeout, &outInfo);

        REQUIRE(result.err == EISErr_Ok);
        REQUIRE(result.service == EISService_Utils);

        CHECK(outInfo.connectionString != nullptr);

        CHECK(expectedConnStrStream.str() == outInfo.connectionString);

        CHECK(outInfo.authType == ADUC_AuthType_SASToken);
        CHECK(outInfo.connType == ADUC_ConnType_Device);

        ADUC_ConnectionInfo_DeAlloc(&outInfo);
    }

    SECTION("RequestConnectionStringFromEISWithExpiry DeviceId , Cert Success Test")
    {
        const auto expiry = static_cast<time_t>(time(nullptr) + 86400); // Expiry is one day after the unit test is run
        uint32_t timeout = 5000;

        // Dynamically build the connectionString with the proper value
        std::stringstream expectedConnStrStream;
        expectedConnStrStream << "HostName=foo.example-devices.net;"
                              << "DeviceId=user-test-device;"
                              << "x509=true";

        std::string expectedCertificateString{ "HikdasdfasdWErfrasdaxcasdfasdf....ASDFASDFAWefsdafdv" };

        // Note: These do not need to be freed! They are returned by the RequestSignature/IdentityFromEIS
        // functions. They are freed within the code block
        REQUIRE(mallocAndStrcpy_s(&g_identityResp, validDeviceX509IdentityResponseStr) == 0);
        g_signatureResp = nullptr;
        REQUIRE(mallocAndStrcpy_s(&g_certificateResp, validCertificateResponseStr) == 0);

        ADUC_ConnectionInfo outInfo = {
            ADUC_AuthType_NotSet, ADUC_ConnType_NotSet, nullptr, nullptr, nullptr, nullptr
        };

        EISUtilityResult result = RequestConnectionStringFromEISWithExpiry(expiry, timeout, &outInfo);

        REQUIRE(result.err == EISErr_Ok);
        REQUIRE(result.service == EISService_Utils);

        CHECK(outInfo.connectionString != nullptr);

        CHECK(expectedConnStrStream.str() == outInfo.connectionString);

        CHECK(outInfo.certificateString == expectedCertificateString);

        CHECK(outInfo.authType == ADUC_AuthType_SASCert);
        CHECK(outInfo.connType == ADUC_ConnType_Device);

        ADUC_ConnectionInfo_DeAlloc(&outInfo);
    }

    SECTION("RequestConnectionStringFromEISWithExpiry ModuleId , SAS Token Success Test")
    {
        const auto expiry = static_cast<time_t>(time(nullptr) + 86400); // Expiry is one day after the unit test is run
        uint32_t timeout = 5000;

        // Dynamically build the connectionString with the proper value
        std::stringstream expectedConnStrStream;
        expectedConnStrStream << "HostName=foo.example-devices.net;"
                              << "DeviceId=user-test-device;"
                              << "ModuleId=user-test-module;"
                              << "SharedAccessSignature=SharedAccessSignature "
                              << "sr=foo.example-devices.net/devices/user-test-device"
                              << "/modules/user-test-module&"
                              << "sig=hIuFfERqcDBnu84EwVlF01JfiaRvH6A20dMWQW6T4fg%3d&"
                              << "se=" << expiry;

        // Note: These do not need to be freed! They are returned by the RequestSignature/IdentityFromEIS
        // functions. They are freed within the code block
        REQUIRE(mallocAndStrcpy_s(&g_identityResp, validModuleSasIdentityResponseStr) == 0);
        REQUIRE(mallocAndStrcpy_s(&g_signatureResp, validSignatureResponseStr) == 0);

        ADUC_ConnectionInfo outInfo = {
            ADUC_AuthType_NotSet, ADUC_ConnType_NotSet, nullptr, nullptr, nullptr, nullptr
        };

        EISUtilityResult result = RequestConnectionStringFromEISWithExpiry(expiry, timeout, &outInfo);

        REQUIRE(result.err == EISErr_Ok);
        REQUIRE(result.service == EISService_Utils);

        CHECK(outInfo.connectionString != nullptr);

        CHECK(expectedConnStrStream.str() == outInfo.connectionString);

        CHECK(outInfo.authType == ADUC_AuthType_SASToken);
        CHECK(outInfo.connType == ADUC_ConnType_Module);

        ADUC_ConnectionInfo_DeAlloc(&outInfo);
    }

    SECTION("RequestConnectionStringFromEISWithExpiry ModuleId , Cert Success Test")
    {
        const auto expiry = static_cast<time_t>(time(nullptr) + 86400); // Expiry is one day after the unit test is run
        uint32_t timeout = 5000;

        // Dynamically build the connectionString with the proper value
        std::stringstream expectedConnStrStream;
        expectedConnStrStream << "HostName=foo.example-devices.net;"
                              << "DeviceId=user-test-device;"
                              << "ModuleId=user-test-module;"
                              << "x509=true";

        std::string expectedCertificateString{ "HikdasdfasdWErfrasdaxcasdfasdf....ASDFASDFAWefsdafdv" };

        // Note: These do not need to be freed! They are returned by the RequestSignature/IdentityFromEIS
        // functions. They are freed within the code block
        REQUIRE(mallocAndStrcpy_s(&g_identityResp, validModuleX509IdentityResponseStr) == 0);
        g_signatureResp = nullptr;
        REQUIRE(mallocAndStrcpy_s(&g_certificateResp, validCertificateResponseStr) == 0);

        ADUC_ConnectionInfo outInfo = {
            ADUC_AuthType_NotSet, ADUC_ConnType_NotSet, nullptr, nullptr, nullptr, nullptr
        };

        EISUtilityResult result = RequestConnectionStringFromEISWithExpiry(expiry, timeout, &outInfo);

        REQUIRE(result.err == EISErr_Ok);
        REQUIRE(result.service == EISService_Utils);

        CHECK(outInfo.connectionString != nullptr);

        CHECK(expectedConnStrStream.str() == outInfo.connectionString);

        CHECK(outInfo.certificateString == expectedCertificateString);

        CHECK(outInfo.authType == ADUC_AuthType_SASCert);
        CHECK(outInfo.connType == ADUC_ConnType_Module);

        ADUC_ConnectionInfo_DeAlloc(&outInfo);
    }

    SECTION("RequestConnectionStringFromEISWithExpiry with malformed identityResponse")
    {
        const auto expiry = static_cast<time_t>(time(nullptr) + 86400); // Expiry is one day after the unit test is run
        uint32_t timeout = 5000;

        // Note: These do not need to be freed! They are returned by the RequestSignature/IdentityFromEIS
        // functions. They are freed within the code block
        REQUIRE(mallocAndStrcpy_s(&g_identityResp, invalidIdentityResponseStr) == 0);
        REQUIRE(mallocAndStrcpy_s(&g_signatureResp, validSignatureResponseStr) == 0);

        ADUC_ConnectionInfo outInfo = {
            ADUC_AuthType_NotSet, ADUC_ConnType_NotSet, nullptr, nullptr, nullptr, nullptr
        };

        EISUtilityResult result = RequestConnectionStringFromEISWithExpiry(expiry, timeout, &outInfo);

        CHECK(outInfo.connectionString == nullptr);

        CHECK(result.service == EISService_IdentityService);
        CHECK(result.err == EISErr_InvalidJsonRespErr);

        CHECK(outInfo.authType == ADUC_AuthType_NotSet);
        CHECK(outInfo.connType == ADUC_ConnType_NotSet);
    }

    SECTION("RequestConnectionStringFromEISWithExpiry with malformed signature response")
    {
        const auto expiry = static_cast<time_t>(time(nullptr) + 86400); // Expiry is one day after the unit test is run
        uint32_t timeout = 5000;

        // Note: These do not need to be freed! They are returned by the RequestSignature/IdentityFromEIS
        // functions. They are freed within the code block
        REQUIRE(mallocAndStrcpy_s(&g_identityResp, validDeviceSasIdentityResponseStr) == 0);
        REQUIRE(mallocAndStrcpy_s(&g_signatureResp, invalidSignatureResponseStr) == 0);

        ADUC_ConnectionInfo outInfo = {
            ADUC_AuthType_NotSet, ADUC_ConnType_NotSet, nullptr, nullptr, nullptr, nullptr
        };

        EISUtilityResult result = RequestConnectionStringFromEISWithExpiry(expiry, timeout, &outInfo);

        CHECK(outInfo.connectionString == nullptr);

        CHECK(result.service == EISService_KeyService);
        CHECK(result.err == EISErr_InvalidJsonRespErr);

        CHECK(outInfo.authType == ADUC_AuthType_NotSet);
        CHECK(outInfo.connType == ADUC_ConnType_NotSet);

        ADUC_ConnectionInfo_DeAlloc(&outInfo);
    }

    SECTION("RequestConnectionStringFromEISWithExpiry with malformed certificate response")
    {
        const auto expiry = static_cast<time_t>(time(nullptr) + 86400); // Expiry is one day after the unit test is run
        uint32_t timeout = 5000;

        // Note: These do not need to be freed! They are returned by the RequestSignature/IdentityFromEIS
        // functions. They are freed within the code block
        REQUIRE(mallocAndStrcpy_s(&g_identityResp, validDeviceX509IdentityResponseStr) == 0);
        g_signatureResp = nullptr;
        REQUIRE(mallocAndStrcpy_s(&g_certificateResp, invalidCertificateResponseStr) == 0);

        ADUC_ConnectionInfo outInfo = {
            ADUC_AuthType_NotSet, ADUC_ConnType_NotSet, nullptr, nullptr, nullptr, nullptr
        };

        EISUtilityResult result = RequestConnectionStringFromEISWithExpiry(expiry, timeout, &outInfo);

        CHECK(outInfo.connectionString == nullptr);

        CHECK(result.service == EISService_CertService);
        CHECK(result.err == EISErr_InvalidJsonRespErr);

        CHECK(outInfo.authType == ADUC_AuthType_NotSet);
        CHECK(outInfo.connType == ADUC_ConnType_NotSet);

        ADUC_ConnectionInfo_DeAlloc(&outInfo);
    }

    SECTION("RequestConnectionStringFromEISWithExpiry with valid timeout and invalid expiry")
    {
        const time_t expiry = time(nullptr);
        uint32_t timeout = 5000;

        // Note: These DO need to be freed. Parameter validation leads to early exit from
        // RequestConnectionStringFromEISWithExpiry
        REQUIRE(mallocAndStrcpy_s(&g_identityResp, validDeviceSasIdentityResponseStr) == 0);
        REQUIRE(mallocAndStrcpy_s(&g_signatureResp, validSignatureResponseStr) == 0);

        ADUC_ConnectionInfo outInfo = {
            ADUC_AuthType_NotSet, ADUC_ConnType_NotSet, nullptr, nullptr, nullptr, nullptr
        };

        EISUtilityResult result = RequestConnectionStringFromEISWithExpiry(expiry, timeout, &outInfo);

        CHECK(outInfo.connectionString == nullptr);

        CHECK(result.service == EISService_Utils);
        CHECK(result.err == EISErr_InvalidArg);

        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc): g_identityResp is a basic C-string so it must be freed by a call to free()
        free(g_identityResp);

        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc): g_signatureResp is a basic C-string so it must be freed by a call to free()
        free(g_signatureResp);

        ADUC_ConnectionInfo_DeAlloc(&outInfo);
    }

    SECTION("RequestConnectionStringFromEISWithExpiry with invalid timeout and valid expiry")
    {
        const auto expiry = static_cast<time_t>(time(nullptr) + 86400); // Expiry is one day after the unit test is run
        uint32_t timeout = 0;

        // Note: These DO need to be freed. Parameter validation leads to early exit from
        // RequestConnectionStringFromEISWithExpiry
        REQUIRE(mallocAndStrcpy_s(&g_identityResp, validDeviceSasIdentityResponseStr) == 0);
        REQUIRE(mallocAndStrcpy_s(&g_signatureResp, validSignatureResponseStr) == 0);

        ADUC_ConnectionInfo outInfo = {
            ADUC_AuthType_NotSet, ADUC_ConnType_NotSet, nullptr, nullptr, nullptr, nullptr
        };

        EISUtilityResult result = RequestConnectionStringFromEISWithExpiry(expiry, timeout, &outInfo);

        CHECK(outInfo.connectionString == nullptr);

        CHECK(result.service == EISService_Utils);
        CHECK(result.err == EISErr_InvalidArg);

        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc): g_identityResp is a basic C-string so it must be freed by a call to free()
        free(g_identityResp);

        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc): g_signatureResp is a basic C-string so it must be freed by a call to free()
        free(g_signatureResp);

        ADUC_ConnectionInfo_DeAlloc(&outInfo);
    }
}
