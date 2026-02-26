/**
 * @file mock_diff_api.cpp
 * @brief Mock implementations of diff API functions and aduc::SharedLib for utils.cpp tests.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_diff_api.h"
#include "aduc/shared_lib.hpp"
#include <cstring>
#include <stdexcept>
#include <string>

MockDiffApiState g_mockDiffApi{};

void ResetDiffApiMocks()
{
    memset(&g_mockDiffApi, 0, sizeof(g_mockDiffApi));

    // Defaults for a working session
    static int dummySession = 1;
    g_mockDiffApi.createSessionResult = &dummySession;
    g_mockDiffApi.errorText = "mock error";
}

// -------------------------------------------------------
// Mock diff API C functions
// -------------------------------------------------------

extern "C"
{

void* mock_create_session()
{
    g_mockDiffApi.createSessionCallCount++;
    return g_mockDiffApi.createSessionResult;
}

void mock_close_session(void* /*handle*/)
{
    g_mockDiffApi.closeSessionCallCount++;
}

int mock_apply(void* /*session*/, const char* /*source*/, const char* /*delta*/, const char* /*target*/)
{
    g_mockDiffApi.applyCallCount++;
    return g_mockDiffApi.applyResult;
}

size_t mock_get_error_count(void* /*handle*/)
{
    return g_mockDiffApi.errorCount;
}

const char* mock_get_error_text(void* /*handle*/, size_t /*index*/)
{
    return g_mockDiffApi.errorText;
}

int mock_get_error_code(void* /*handle*/, size_t /*index*/)
{
    return g_mockDiffApi.errorCode;
}

} // extern "C"

// -------------------------------------------------------
// Mock aduc::SharedLib implementation
// -------------------------------------------------------

namespace aduc
{

SharedLib::SharedLib(const std::string& /*libPath*/)
{
    if (g_mockDiffApi.sharedLibShouldThrow)
    {
        throw std::runtime_error("mock: dlopen failed");
    }

    libHandle = reinterpret_cast<void*>(0x1); // non-null sentinel
}

SharedLib::~SharedLib()
{
    libHandle = nullptr;
}

void SharedLib::EnsureSymbols(std::vector<std::string> /*symbols*/) const
{
    if (g_mockDiffApi.ensureSymbolsShouldThrow)
    {
        throw std::runtime_error("mock: symbol not found");
    }
}

void* SharedLib::GetSymbol(const std::string& symbol) const
{
    if (symbol == "adu_diff_apply_create_session")
    {
        return reinterpret_cast<void*>(&mock_create_session);
    }
    if (symbol == "adu_diff_apply_close_session")
    {
        return reinterpret_cast<void*>(&mock_close_session);
    }
    if (symbol == "adu_diff_apply")
    {
        return reinterpret_cast<void*>(&mock_apply);
    }
    if (symbol == "adu_diff_apply_get_error_count")
    {
        return reinterpret_cast<void*>(&mock_get_error_count);
    }
    if (symbol == "adu_diff_apply_get_error_text")
    {
        return reinterpret_cast<void*>(&mock_get_error_text);
    }
    if (symbol == "adu_diff_apply_get_error_code")
    {
        return reinterpret_cast<void*>(&mock_get_error_code);
    }

    throw std::runtime_error("mock: unknown symbol: " + symbol);
}

} // namespace aduc
