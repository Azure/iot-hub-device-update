/**
 * @file mock_diff_api.h
 * @brief Configurable mock state for utils.cpp ProcessDeltaUpdate test.
 *
 * Provides mock implementations of the diff API functions that
 * ProcessDeltaUpdate loads dynamically via SharedLib, and a mock SharedLib.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef MOCK_DIFF_API_H
#define MOCK_DIFF_API_H

#include <cstddef>

struct MockDiffApiState
{
    /** Whether SharedLib constructor should throw (simulating dlopen failure) */
    bool sharedLibShouldThrow;

    /** Whether EnsureSymbols should throw (simulating missing symbol) */
    bool ensureSymbolsShouldThrow;

    /** adu_diff_apply_create_session mock: NULL means session creation failed */
    void* createSessionResult;
    int createSessionCallCount;

    /** adu_diff_apply mock: 0 = success, non-zero = error */
    int applyResult;
    int applyCallCount;

    /** adu_diff_apply_close_session mock */
    int closeSessionCallCount;

    /** Error reporting */
    size_t errorCount;
    int errorCode;
    const char* errorText;
};

extern MockDiffApiState g_mockDiffApi;

void ResetDiffApiMocks();

// Mock diff API C functions (returned by mock GetSymbol)
extern "C"
{
    void* mock_create_session();
    void mock_close_session(void* handle);
    int mock_apply(void* session, const char* source, const char* delta, const char* target);
    size_t mock_get_error_count(void* handle);
    const char* mock_get_error_text(void* handle, size_t index);
    int mock_get_error_code(void* handle, size_t index);
}

#endif /* MOCK_DIFF_API_H */
