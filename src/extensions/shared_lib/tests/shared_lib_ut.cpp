/**
 * @file shared_lib_ut.cpp
 * @brief Unit Tests for SharedLib class.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/shared_lib.hpp"
#include "aduc/plugin_exception.hpp"

#include <catch2/catch_all.hpp>

#include <aduc/result.h>
#include <stdexcept>
#include <string>
#include <vector>

// =====================================================================
// SharedLib constructor tests
// =====================================================================

TEST_CASE("SharedLib constructor throws on invalid library path")
{
    CHECK_THROWS_AS(aduc::SharedLib("/nonexistent/path/libfake.so"), std::runtime_error);
}

TEST_CASE("SharedLib constructor succeeds with empty path (dlopen self)")
{
    // dlopen("") opens the main program handle — this is valid behavior
    CHECK_NOTHROW(aduc::SharedLib(""));
}

TEST_CASE("SharedLib constructor succeeds with valid system library")
{
    // Use versioned .so — unversioned libm.so is a linker script on many distros
    CHECK_NOTHROW(aduc::SharedLib("libm.so.6"));
}

TEST_CASE("SharedLib constructor succeeds with libc")
{
    CHECK_NOTHROW(aduc::SharedLib("libc.so.6"));
}

// =====================================================================
// GetSymbol tests
// =====================================================================

TEST_CASE("SharedLib GetSymbol returns non-null for existing symbol")
{
    aduc::SharedLib lib("libm.so.6");

    // 'cos' is a standard math function in libm
    void* sym = lib.GetSymbol("cos");
    CHECK(sym != nullptr);
}

TEST_CASE("SharedLib GetSymbol succeeds for multiple math functions")
{
    aduc::SharedLib lib("libm.so.6");

    CHECK(lib.GetSymbol("sin") != nullptr);
    CHECK(lib.GetSymbol("cos") != nullptr);
    CHECK(lib.GetSymbol("sqrt") != nullptr);
    CHECK(lib.GetSymbol("floor") != nullptr);
}

TEST_CASE("SharedLib GetSymbol throws on non-existent symbol")
{
    aduc::SharedLib lib("libm.so.6");

    CHECK_THROWS_AS(lib.GetSymbol("this_symbol_does_not_exist_xyz"), std::runtime_error);
}

// =====================================================================
// EnsureSymbols tests
// =====================================================================

TEST_CASE("SharedLib EnsureSymbols succeeds with valid symbols")
{
    aduc::SharedLib lib("libm.so.6");

    std::vector<std::string> symbols{ "sin", "cos", "sqrt" };
    CHECK_NOTHROW(lib.EnsureSymbols(symbols));
}

TEST_CASE("SharedLib EnsureSymbols throws when any symbol is missing")
{
    aduc::SharedLib lib("libm.so.6");

    std::vector<std::string> symbols{ "sin", "nonexistent_symbol_abc", "cos" };
    CHECK_THROWS_AS(lib.EnsureSymbols(symbols), std::runtime_error);
}

TEST_CASE("SharedLib EnsureSymbols succeeds with empty symbol list")
{
    aduc::SharedLib lib("libm.so.6");

    std::vector<std::string> symbols{};
    CHECK_NOTHROW(lib.EnsureSymbols(symbols));
}

// =====================================================================
// PluginException tests
// =====================================================================

TEST_CASE("PluginException stores message and symbol")
{
    aduc::PluginException ex("test error message", "TestSymbolName");

    CHECK(std::string(ex.what()) == "test error message");
    CHECK(ex.Symbol() == "TestSymbolName");
}

TEST_CASE("PluginException with empty symbol")
{
    aduc::PluginException ex("some error", "");

    CHECK(std::string(ex.what()) == "some error");
    CHECK(ex.Symbol().empty());
}

// =====================================================================
// SharedLib destructor safety
// =====================================================================

TEST_CASE("SharedLib can be constructed and destructed in a scope")
{
    // Verify no crash on scope exit / destructor
    {
        aduc::SharedLib lib("libm.so.6");
        void* sym = lib.GetSymbol("cos");
        CHECK(sym != nullptr);
    }
    // If we get here, destructor ran without crashing.
    CHECK(true);
}

// =====================================================================
// plugin_call_helper.hpp tests
// =====================================================================

// Provide stub macros required by plugin_call_helper.hpp
#ifndef UNREFERENCED_PARAMETER
#    define UNREFERENCED_PARAMETER(param) (void)(param)
#endif
#ifndef Log_Debug
#    define Log_Debug(...) ((void)0)
#endif
#ifndef Log_Error
#    define Log_Error(...) ((void)0)
#endif

#include "aduc/plugin_call_helper.hpp"

// Test function that returns an ADUC_Result
static ADUC_Result TestFuncReturnsResult(int val)
{
    return ADUC_Result{ val, val * 10 };
}

// Test function with void return
static int g_voidFuncCalled = 0;
static void TestFuncVoid(int val)
{
    (void)val;
    g_voidFuncCalled++;
}

// Test function that throws std::exception
static ADUC_Result TestFuncThrowsStdException(int)
{
    throw std::runtime_error("test exception from func");
}

// Test function that throws non-std exception
static ADUC_Result TestFuncThrowsNonStd(int)
{
    throw 42;
}

TEST_CASE("CallExportHandlerInternal with ExportReturnsAducResult=true calls fn and stores result")
{
    ADUC_Result result{};
    void* sym = reinterpret_cast<void*>(&TestFuncReturnsResult);
    CallExportHandlerInternal<ADUC_Result (*)(int)>(IntToType<true>(), sym, &result, 7);
    CHECK(result.ResultCode == 7);
    CHECK(result.ExtendedResultCode == 70);
}

TEST_CASE("CallExportHandlerInternal with ExportReturnsAducResult=false calls fn without setting result")
{
    g_voidFuncCalled = 0;
    ADUC_Result result{};
    void* sym = reinterpret_cast<void*>(&TestFuncVoid);
    CallExportHandlerInternal<void (*)(int)>(IntToType<false>(), sym, &result, 99);
    CHECK(g_voidFuncCalled == 1);
    // result should be unchanged (zero-initialized)
    CHECK(result.ResultCode == 0);
}

TEST_CASE("CallExportHandler dispatches to true path correctly")
{
    ADUC_Result result{};
    void* sym = reinterpret_cast<void*>(&TestFuncReturnsResult);
    CallExportHandler<ADUC_Result (*)(int), true>(sym, &result, 3);
    CHECK(result.ResultCode == 3);
    CHECK(result.ExtendedResultCode == 30);
}

TEST_CASE("CallExportHandler dispatches to false path correctly")
{
    g_voidFuncCalled = 0;
    ADUC_Result result{};
    void* sym = reinterpret_cast<void*>(&TestFuncVoid);
    CallExportHandler<void (*)(int), false>(sym, &result, 5);
    CHECK(g_voidFuncCalled == 1);
}

TEST_CASE("CallExport with non-existent symbol throws PluginException")
{
    aduc::SharedLib lib("libm.so.6");
    ADUC_Result result{};
    CHECK_THROWS_AS(
        (CallExport<ADUC_Result (*)(int), true>("nonexistent_symbol_xyz", lib, &result, 0)),
        aduc::PluginException);
}

TEST_CASE("CallExport: resolved function that throws std::exception is caught")
{
    // We need a SharedLib with a known symbol. Open the test executable itself.
    // Since the test symbol might not be exported, we use a different approach:
    // directly test CallExportHandler with a throwing function.
    ADUC_Result result{};
    void* sym = reinterpret_cast<void*>(&TestFuncThrowsStdException);
    // CallExportHandler itself doesn't catch — but we verify the call path.
    CHECK_THROWS_AS(
        (CallExportHandler<ADUC_Result (*)(int), true>(sym, &result, 0)),
        std::runtime_error);
}

TEST_CASE("CallExport: resolved function that throws non-std exception")
{
    ADUC_Result result{};
    void* sym = reinterpret_cast<void*>(&TestFuncThrowsNonStd);
    CHECK_THROWS(
        (CallExportHandler<ADUC_Result (*)(int), true>(sym, &result, 0)));
}

// =====================================================================
// Exported C functions for CallExport integration tests.
// These are visible to dlsym via -rdynamic link flag.
// =====================================================================

extern "C" ADUC_Result TestExportedReturnsResult(int val)
{
    return ADUC_Result{ val, val * 10 };
}

extern "C" void TestExportedVoidFunc(int val)
{
    (void)val;
}

extern "C" ADUC_Result TestExportedThrowsStd(int)
{
    throw std::runtime_error("exported func throws std::exception");
}

extern "C" ADUC_Result TestExportedThrowsNonStd(int)
{
    throw 42;
}

// =====================================================================
// CallExport integration tests (exercises the full CallExport template
// with real dlsym symbol resolution via SharedLib(""))
// =====================================================================

TEST_CASE("CallExport happy path: resolves symbol and calls function returning ADUC_Result")
{
    aduc::SharedLib self("");
    ADUC_Result result{};
    CallExport<ADUC_Result (*)(int), true>("TestExportedReturnsResult", self, &result, 7);
    CHECK(result.ResultCode == 7);
    CHECK(result.ExtendedResultCode == 70);
}

TEST_CASE("CallExport happy path: resolves symbol and calls void function")
{
    aduc::SharedLib self("");
    ADUC_Result result{};
    // void dispatch; result should remain zero-initialized
    CallExport<void (*)(int), false>("TestExportedVoidFunc", self, &result, 42);
    CHECK(result.ResultCode == 0);
}

TEST_CASE("CallExport catches std::exception thrown by resolved function")
{
    aduc::SharedLib self("");
    ADUC_Result result{};
    // CallExport catches the exception internally; should NOT propagate
    CHECK_NOTHROW(
        (CallExport<ADUC_Result (*)(int), true>("TestExportedThrowsStd", self, &result, 0)));
}

TEST_CASE("CallExport catches non-std exception thrown by resolved function")
{
    aduc::SharedLib self("");
    ADUC_Result result{};
    CHECK_NOTHROW(
        (CallExport<ADUC_Result (*)(int), true>("TestExportedThrowsNonStd", self, &result, 0)));
}
