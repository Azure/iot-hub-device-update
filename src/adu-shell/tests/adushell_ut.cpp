/**
 * @file adushell_ut.cpp
 * @brief Unit tests for adu-shell public task result type.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "adushell.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("ADUShellTaskResult defaults to success and empty output")
{
    ADUShellTaskResult result;

    CHECK(result.ExitStatus() == EXIT_SUCCESS);
    CHECK(result.Output().empty());
}

TEST_CASE("ADUShellTaskResult stores updated exit status")
{
    ADUShellTaskResult result;

    result.SetExitStatus(ADUSHELL_EXIT_UNSUPPORTED);
    CHECK(result.ExitStatus() == ADUSHELL_EXIT_UNSUPPORTED);

    result.SetExitStatus(42);
    CHECK(result.ExitStatus() == 42);
}

TEST_CASE("ADUShellTaskResult output is mutable and preserved")
{
    ADUShellTaskResult result;

    result.Output() = "line1";
    result.Output() += "\nline2";

    CHECK(result.Output() == "line1\nline2");
}
