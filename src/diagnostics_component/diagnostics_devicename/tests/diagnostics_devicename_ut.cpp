/**
 * @file diagnostics_devicename_ut.cpp
 * @brief Unit Tests for the Diagnostics Device Name module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "diagnostics_devicename.h"

#include <azure_c_shared_utility/strings.h>
#include <catch2/catch_all.hpp>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

//
// --wrap mock infrastructure
//
// The linker --wrap flag redirects calls to STRING_new, STRING_sprintf, and
// mallocAndStrcpy_s through the __wrap_ variants below. When the corresponding
// g_fail_* flag is set the wrapper returns a failure code; otherwise it
// delegates to the real implementation via the __real_ symbol.
//

extern "C"
{
    // Real function declarations (provided by the linker via --wrap)
    STRING_HANDLE __real_STRING_new(void);
    int __real_mallocAndStrcpy_s(char** destination, const char* source);

    // Mock control flags — reset before each mock-dependent test
    static bool g_fail_STRING_new = false;
    static bool g_fail_STRING_sprintf = false;
    static bool g_fail_mallocAndStrcpy_s = false;

    STRING_HANDLE __wrap_STRING_new(void)
    {
        if (g_fail_STRING_new)
        {
            return NULL;
        }
        return __real_STRING_new();
    }

    int __wrap_STRING_sprintf(STRING_HANDLE handle, const char* format, ...)
    {
        if (g_fail_STRING_sprintf)
        {
            return 1; // non-zero signals failure
        }

        // Forward to a real implementation: format into a local buffer, then
        // set the STRING_HANDLE content via STRING_copy (non-variadic).
        va_list args;
        va_start(args, format);
        char buf[1024];
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);

        return STRING_copy(handle, buf);
    }

    int __wrap_mallocAndStrcpy_s(char** destination, const char* source)
    {
        if (g_fail_mallocAndStrcpy_s)
        {
            return 1; // non-zero signals failure
        }
        return __real_mallocAndStrcpy_s(destination, source);
    }
}

// RAII helper to reset mock flags and clean up device name state
class DiagnosticsDeviceNameTestHelper
{
public:
    DiagnosticsDeviceNameTestHelper()
    {
        g_fail_STRING_new = false;
        g_fail_STRING_sprintf = false;
        g_fail_mallocAndStrcpy_s = false;
    }

    ~DiagnosticsDeviceNameTestHelper()
    {
        g_fail_STRING_new = false;
        g_fail_STRING_sprintf = false;
        g_fail_mallocAndStrcpy_s = false;
        DiagnosticsComponent_DestroyDeviceName();
    }
};

// ===========================================================================
// Existing functional tests
// ===========================================================================

TEST_CASE("DiagnosticsComponent_SetDeviceName")
{
    DiagnosticsDeviceNameTestHelper helper;

    SECTION("SetDeviceName with valid deviceId only")
    {
        const char* deviceId = "test-device-id";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId, nullptr));

        char* retrievedName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName));
        REQUIRE(retrievedName != nullptr);
        CHECK(strcmp(retrievedName, "test-device-id") == 0);

        free(retrievedName);
    }

    SECTION("SetDeviceName with valid deviceId and moduleId")
    {
        DiagnosticsComponent_DestroyDeviceName(); // Reset state

        const char* deviceId = "test-device-id";
        const char* moduleId = "test-module-id";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId, moduleId));

        char* retrievedName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName));
        REQUIRE(retrievedName != nullptr);
        CHECK(strcmp(retrievedName, "test-device-id/test-module-id") == 0);

        free(retrievedName);
    }

    SECTION("SetDeviceName with null deviceId fails")
    {
        CHECK_FALSE(DiagnosticsComponent_SetDeviceName(nullptr, nullptr));
    }

    SECTION("SetDeviceName with null deviceId and valid moduleId fails")
    {
        CHECK_FALSE(DiagnosticsComponent_SetDeviceName(nullptr, "module-id"));
    }

    SECTION("SetDeviceName can be called multiple times after destroy")
    {
        const char* deviceId1 = "device-1";
        const char* deviceId2 = "device-2";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId1, nullptr));

        char* retrievedName1 = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName1));
        REQUIRE(retrievedName1 != nullptr);
        CHECK(strcmp(retrievedName1, "device-1") == 0);
        free(retrievedName1);

        DiagnosticsComponent_DestroyDeviceName();

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId2, nullptr));

        char* retrievedName2 = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName2));
        REQUIRE(retrievedName2 != nullptr);
        CHECK(strcmp(retrievedName2, "device-2") == 0);
        free(retrievedName2);
    }

    SECTION("SetDeviceName with empty deviceId")
    {
        DiagnosticsComponent_DestroyDeviceName();

        const char* deviceId = "";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId, nullptr));

        char* retrievedName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName));
        REQUIRE(retrievedName != nullptr);
        CHECK(strcmp(retrievedName, "") == 0);

        free(retrievedName);
    }

    SECTION("SetDeviceName with empty moduleId")
    {
        DiagnosticsComponent_DestroyDeviceName();

        const char* deviceId = "test-device";
        const char* moduleId = "";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId, moduleId));

        char* retrievedName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName));
        REQUIRE(retrievedName != nullptr);
        CHECK(strcmp(retrievedName, "test-device/") == 0);

        free(retrievedName);
    }
}

TEST_CASE("DiagnosticsComponent_SetDeviceName - Reuse without destroy")
{
    DiagnosticsDeviceNameTestHelper helper;

    SECTION("SetDeviceName called twice without destroy succeeds")
    {
        const char* deviceId1 = "first-device";
        const char* deviceId2 = "second-device";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId1, nullptr));

        char* retrievedName1 = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName1));
        REQUIRE(retrievedName1 != nullptr);
        CHECK(strcmp(retrievedName1, "first-device") == 0);
        free(retrievedName1);

        // Call SetDeviceName again without destroying - should reuse the existing handle
        CHECK(DiagnosticsComponent_SetDeviceName(deviceId2, nullptr));

        char* retrievedName2 = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName2));
        REQUIRE(retrievedName2 != nullptr);
        CHECK(strcmp(retrievedName2, "second-device") == 0);
        free(retrievedName2);
    }

    SECTION("SetDeviceName called twice with moduleId without destroy")
    {
        const char* deviceId1 = "device-a";
        const char* moduleId1 = "module-a";
        const char* deviceId2 = "device-b";
        const char* moduleId2 = "module-b";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId1, moduleId1));

        char* retrievedName1 = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName1));
        REQUIRE(retrievedName1 != nullptr);
        CHECK(strcmp(retrievedName1, "device-a/module-a") == 0);
        free(retrievedName1);

        // Call SetDeviceName again without destroying
        CHECK(DiagnosticsComponent_SetDeviceName(deviceId2, moduleId2));

        char* retrievedName2 = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName2));
        REQUIRE(retrievedName2 != nullptr);
        CHECK(strcmp(retrievedName2, "device-b/module-b") == 0);
        free(retrievedName2);
    }

    SECTION("SetDeviceName with different moduleId combinations without destroy")
    {
        // First call with moduleId
        CHECK(DiagnosticsComponent_SetDeviceName("dev1", "mod1"));

        char* name1 = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&name1));
        CHECK(strcmp(name1, "dev1/mod1") == 0);
        free(name1);

        // Second call without moduleId
        CHECK(DiagnosticsComponent_SetDeviceName("dev2", nullptr));

        char* name2 = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&name2));
        CHECK(strcmp(name2, "dev2") == 0);
        free(name2);

        // Third call with moduleId again
        CHECK(DiagnosticsComponent_SetDeviceName("dev3", "mod3"));

        char* name3 = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&name3));
        CHECK(strcmp(name3, "dev3/mod3") == 0);
        free(name3);
    }
}

TEST_CASE("DiagnosticsComponent_GetDeviceName")
{
    DiagnosticsDeviceNameTestHelper helper;

    SECTION("GetDeviceName after SetDeviceName")
    {
        const char* deviceId = "my-device";
        const char* moduleId = "my-module";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId, moduleId));

        char* deviceName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&deviceName));
        REQUIRE(deviceName != nullptr);
        CHECK(strcmp(deviceName, "my-device/my-module") == 0);

        free(deviceName);
    }

    SECTION("GetDeviceName can be called multiple times")
    {
        CHECK(DiagnosticsComponent_SetDeviceName("test-device", "test-module"));

        // Call GetDeviceName multiple times
        for (int i = 0; i < 3; ++i)
        {
            char* deviceName = nullptr;
            CHECK(DiagnosticsComponent_GetDeviceName(&deviceName));
            REQUIRE(deviceName != nullptr);
            CHECK(strcmp(deviceName, "test-device/test-module") == 0);
            free(deviceName);
        }
    }

    SECTION("GetDeviceName with long device and module names")
    {
        // Create a longer device/module name to test string handling
        const char* longDeviceId = "this-is-a-longer-device-identifier-for-testing";
        const char* longModuleId = "this-is-a-longer-module-identifier-for-testing";

        CHECK(DiagnosticsComponent_SetDeviceName(longDeviceId, longModuleId));

        char* deviceName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&deviceName));
        REQUIRE(deviceName != nullptr);

        // Build expected string
        std::string expected = std::string(longDeviceId) + "/" + std::string(longModuleId);
        CHECK(strcmp(deviceName, expected.c_str()) == 0);

        free(deviceName);
    }
}

TEST_CASE("DiagnosticsComponent_DestroyDeviceName")
{
    SECTION("DestroyDeviceName can be called multiple times safely")
    {
        DiagnosticsComponent_SetDeviceName("device", nullptr);
        DiagnosticsComponent_DestroyDeviceName();
        DiagnosticsComponent_DestroyDeviceName(); // Should not crash
    }

    SECTION("DestroyDeviceName resets state for new SetDeviceName")
    {
        const char* firstDevice = "first-device";
        const char* secondDevice = "second-device";

        CHECK(DiagnosticsComponent_SetDeviceName(firstDevice, nullptr));
        DiagnosticsComponent_DestroyDeviceName();

        CHECK(DiagnosticsComponent_SetDeviceName(secondDevice, nullptr));

        char* deviceName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&deviceName));
        REQUIRE(deviceName != nullptr);
        CHECK(strcmp(deviceName, "second-device") == 0);

        free(deviceName);
        DiagnosticsComponent_DestroyDeviceName();
    }
}

TEST_CASE("DiagnosticsComponent_GetDeviceName - Edge Cases")
{
    SECTION("GetDeviceName before SetDeviceName returns false")
    {
        // Ensure clean state - no device name set
        DiagnosticsComponent_DestroyDeviceName();

        char* deviceName = nullptr;
        // When s_DiagnosticsDeviceName is NULL, STRING_c_str returns NULL
        // and mallocAndStrcpy_s should fail or handle gracefully
        CHECK_FALSE(DiagnosticsComponent_GetDeviceName(&deviceName));
    }

    SECTION("GetDeviceName after DestroyDeviceName returns false")
    {
        // Set a device name first
        CHECK(DiagnosticsComponent_SetDeviceName("test-device", nullptr));

        // Verify it works
        char* deviceName1 = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&deviceName1));
        REQUIRE(deviceName1 != nullptr);
        free(deviceName1);

        // Destroy it
        DiagnosticsComponent_DestroyDeviceName();

        // Now GetDeviceName should fail
        char* deviceName2 = nullptr;
        CHECK_FALSE(DiagnosticsComponent_GetDeviceName(&deviceName2));
    }

    SECTION("GetDeviceName with NULL output parameter returns false")
    {
        DiagnosticsComponent_DestroyDeviceName();
        CHECK(DiagnosticsComponent_SetDeviceName("test-device", nullptr));

        // Passing NULL as the output parameter
        CHECK_FALSE(DiagnosticsComponent_GetDeviceName(nullptr));

        DiagnosticsComponent_DestroyDeviceName();
    }
}

TEST_CASE("DiagnosticsComponent_SetDeviceName - Special Characters and Unicode")
{
    DiagnosticsDeviceNameTestHelper helper;

    SECTION("SetDeviceName with special characters in deviceId")
    {
        const char* deviceId = "device-123_test.name";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId, nullptr));

        char* retrievedName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName));
        REQUIRE(retrievedName != nullptr);
        CHECK(strcmp(retrievedName, deviceId) == 0);

        free(retrievedName);
    }

    SECTION("SetDeviceName with special characters in moduleId")
    {
        DiagnosticsComponent_DestroyDeviceName();

        const char* deviceId = "device-abc";
        const char* moduleId = "module_123.test-name";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId, moduleId));

        char* retrievedName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName));
        REQUIRE(retrievedName != nullptr);

        std::string expected = std::string(deviceId) + "/" + moduleId;
        CHECK(strcmp(retrievedName, expected.c_str()) == 0);

        free(retrievedName);
    }

    SECTION("SetDeviceName with numbers only")
    {
        DiagnosticsComponent_DestroyDeviceName();

        const char* deviceId = "1234567890";
        const char* moduleId = "0987654321";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId, moduleId));

        char* retrievedName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName));
        REQUIRE(retrievedName != nullptr);
        CHECK(strcmp(retrievedName, "1234567890/0987654321") == 0);

        free(retrievedName);
    }

    SECTION("SetDeviceName with hyphen and underscore combinations")
    {
        DiagnosticsComponent_DestroyDeviceName();

        const char* deviceId = "device--double__underscore";

        CHECK(DiagnosticsComponent_SetDeviceName(deviceId, nullptr));

        char* retrievedName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName));
        REQUIRE(retrievedName != nullptr);
        CHECK(strcmp(retrievedName, deviceId) == 0);

        free(retrievedName);
    }

    SECTION("SetDeviceName with very long names")
    {
        DiagnosticsComponent_DestroyDeviceName();

        // Create a reasonably long device name (256 characters)
        std::string longDeviceId(256, 'x');
        std::string longModuleId(256, 'y');

        CHECK(DiagnosticsComponent_SetDeviceName(longDeviceId.c_str(), longModuleId.c_str()));

        char* retrievedName = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName));
        REQUIRE(retrievedName != nullptr);

        std::string expected = longDeviceId + "/" + longModuleId;
        CHECK(strcmp(retrievedName, expected.c_str()) == 0);

        free(retrievedName);
    }
}

TEST_CASE("DiagnosticsComponent - Stress Testing")
{
    DiagnosticsDeviceNameTestHelper helper;

    SECTION("Repeated Set and Get operations")
    {
        for (int i = 0; i < 10; ++i)
        {
            DiagnosticsComponent_DestroyDeviceName();

            std::string deviceId = "device-" + std::to_string(i);
            std::string moduleId = "module-" + std::to_string(i);

            CHECK(DiagnosticsComponent_SetDeviceName(deviceId.c_str(), moduleId.c_str()));

            char* retrievedName = nullptr;
            CHECK(DiagnosticsComponent_GetDeviceName(&retrievedName));
            REQUIRE(retrievedName != nullptr);

            std::string expected = deviceId + "/" + moduleId;
            CHECK(strcmp(retrievedName, expected.c_str()) == 0);

            free(retrievedName);
        }
    }

    SECTION("Repeated Destroy operations are safe")
    {
        CHECK(DiagnosticsComponent_SetDeviceName("test", nullptr));

        // Multiple destroys should be safe
        for (int i = 0; i < 5; ++i)
        {
            DiagnosticsComponent_DestroyDeviceName();
        }

        // Should be able to set again after multiple destroys
        CHECK(DiagnosticsComponent_SetDeviceName("test2", nullptr));

        char* name = nullptr;
        CHECK(DiagnosticsComponent_GetDeviceName(&name));
        REQUIRE(name != nullptr);
        CHECK(strcmp(name, "test2") == 0);
        free(name);
    }
}

// ===========================================================================
// Mock-based tests for failure paths
// ===========================================================================

TEST_CASE("DiagnosticsComponent_SetDeviceName - STRING_new failure")
{
    DiagnosticsDeviceNameTestHelper helper;

    SECTION("SetDeviceName returns false when STRING_new fails")
    {
        g_fail_STRING_new = true;

        CHECK_FALSE(DiagnosticsComponent_SetDeviceName("device-id", nullptr));
    }

    SECTION("SetDeviceName returns false when STRING_new fails with moduleId")
    {
        g_fail_STRING_new = true;

        CHECK_FALSE(DiagnosticsComponent_SetDeviceName("device-id", "module-id"));
    }
}

TEST_CASE("DiagnosticsComponent_SetDeviceName - STRING_sprintf failure")
{
    DiagnosticsDeviceNameTestHelper helper;

    SECTION("SetDeviceName returns false when STRING_sprintf fails without moduleId")
    {
        // STRING_new must succeed so we reach the STRING_sprintf call
        g_fail_STRING_sprintf = true;

        CHECK_FALSE(DiagnosticsComponent_SetDeviceName("device-id", nullptr));
    }

    SECTION("SetDeviceName returns false when STRING_sprintf fails with moduleId")
    {
        g_fail_STRING_sprintf = true;

        CHECK_FALSE(DiagnosticsComponent_SetDeviceName("device-id", "module-id"));
    }
}

TEST_CASE("DiagnosticsComponent_GetDeviceName - mallocAndStrcpy_s failure")
{
    DiagnosticsDeviceNameTestHelper helper;

    SECTION("GetDeviceName returns false when mallocAndStrcpy_s fails")
    {
        // First set a valid device name
        CHECK(DiagnosticsComponent_SetDeviceName("device-id", nullptr));

        // Now make mallocAndStrcpy_s fail
        g_fail_mallocAndStrcpy_s = true;

        char* deviceName = nullptr;
        CHECK_FALSE(DiagnosticsComponent_GetDeviceName(&deviceName));
    }

    SECTION("GetDeviceName returns false when mallocAndStrcpy_s fails with moduleId set")
    {
        CHECK(DiagnosticsComponent_SetDeviceName("device-id", "module-id"));

        g_fail_mallocAndStrcpy_s = true;

        char* deviceName = nullptr;
        CHECK_FALSE(DiagnosticsComponent_GetDeviceName(&deviceName));
    }
}
