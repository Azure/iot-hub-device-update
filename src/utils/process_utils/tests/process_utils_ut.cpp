/**
 * @file process_utils_ut.cpp
 * @brief Unit Tests for process_utils library
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include <catch2/catch.hpp>
#include <unistd.h>
#include <vector>

using Catch::Matchers::Contains;

#include "aduc/process_utils.hpp"

const char* command = "process_utils_tests_helper";

TEST_CASE("Capture exit status", "[!hide][functional_test]")
{
    std::vector<std::string> args;
    args.emplace_back("-e");
    args.emplace_back("This is a standard error string.");
    args.emplace_back("-x");
    args.emplace_back("200");
    std::string output;
    const int exitCode = ADUC_LaunchChildProcess(command, args, output);

    REQUIRE(exitCode == 200);
}

TEST_CASE("apt-get fail")
{
    std::vector<std::string> args;
    args.emplace_back("install");
    args.emplace_back("foopackage");
    std::string output;
    const int exitCode = ADUC_LaunchChildProcess("apt-get", args, output);
    REQUIRE(exitCode == 100);
}

TEST_CASE("Capture standard output", "[!hide][functional_test]")
{
    std::vector<std::string> args;
    args.emplace_back("-o");
    args.emplace_back("This is a normal output string.");
    std::string output;
    ADUC_LaunchChildProcess(command, args, output);

    CHECK_THAT(output.c_str(), Contains("This is a normal output string.\n"));
}

TEST_CASE("Capture standard error", "[!hide][functional_test]")
{
    std::vector<std::string> args;
    args.emplace_back("-e");
    args.emplace_back("This is a standard error string.");
    std::string output;
    ADUC_LaunchChildProcess(command, args, output);

    CHECK_THAT(output.c_str(), Contains("This is a standard error string.\n"));
}

TEST_CASE("hostname error")
{
    const char* bogusOption = "--bogus-param-abc";
    std::vector<std::string> args;
    args.emplace_back(bogusOption);
    std::string output;

    char hostname[1024];
    gethostname(hostname, sizeof(hostname) - 1);

    const int exitCode = ADUC_LaunchChildProcess("hostname", args, output);

    REQUIRE(exitCode != EXIT_SUCCESS);

    // Expecting output text to contain the specified bogus option.
    // e.g., hostname: unrecognized option '--bogus-param-abc'
    CHECK_THAT(output.c_str(), Contains(bogusOption));
}

TEST_CASE("Invalid option - cp -1")
{
    std::string command = "cp";
    std::vector<std::string> args;
    args.emplace_back("-1");
    std::string output;
    ADUC_LaunchChildProcess(command, args, output);

    CHECK_THAT(output.c_str(), Contains("invalid option -- '1'"));
}