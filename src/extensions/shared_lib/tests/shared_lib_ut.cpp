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
