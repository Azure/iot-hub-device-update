/**
 * @file process_utils_ut.cpp
 * @brief Unit Tests for process_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
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

TEST_CASE("VerifyProcessEffectiveGroup")
{
    SECTION("it should return false when gegrnam returns nullptr and sets errno")
    {
        int expectedErrno = EINTR; // signal interrupt

        const std::function<gid_t()> mock_getegid = [&]() { return 101; };

        const std::function<struct group*(const char*)> mock_getgrnam = [expectedErrno](const char*) {
            errno = expectedErrno;
            return nullptr;
        };

        CHECK(!VerifyProcessEffectiveGroup("dontCareGroup" /* groupName */, mock_getegid, mock_getgrnam));
    }

    SECTION("it should return false when gegrnam returns nullptr and does not set errno")
    {
        const std::function<gid_t()> mock_getegid = [&]() { return 101; };

        const std::function<struct group*(const char*)> mock_getgrnam = [](const char*) {
            // do not set errno to signify missing entry in /etc/groups database.
            return nullptr;
        };

        CHECK(!VerifyProcessEffectiveGroup("dontCareGroup" /* groupName */, mock_getegid, mock_getgrnam));
    }

    SECTION("it should return false when not root and not desired group")
    {
        int effectiveProcessGroupId = 100; // not root(0)

        const std::function<gid_t()> mock_getegid = [&]() { return effectiveProcessGroupId; };

        const std::function<struct group*(const char*)> mock_getgrnam = [effectiveProcessGroupId](const char*) {
            static struct group desiredGroupEntry = {};
            desiredGroupEntry.gr_gid = effectiveProcessGroupId + 1; // does not match effective process group id
            return &desiredGroupEntry;
        };

        CHECK(!VerifyProcessEffectiveGroup("desiredGroup" /* groupName */, mock_getegid, mock_getgrnam));
    }

    SECTION("it should return true(success) when root")
    {
        int effectiveProcessGroupId = 0; // root

        const std::function<gid_t()> mock_getegid = [&]() { return effectiveProcessGroupId; };

        const std::function<struct group*(const char*)> mock_getgrnam = [](const char*) {
            static struct group desiredGroupEntry = {};
            desiredGroupEntry.gr_gid = 100; // not root

            return &desiredGroupEntry;
        };

        CHECK(VerifyProcessEffectiveGroup("desiredGroup" /* groupName */, mock_getegid, mock_getgrnam));
    }

    SECTION("it should return true when not root but group matches")
    {
        int effectiveProcessGroupId = 100; // not root(0)
        const gid_t desiredGroupId = 100;

        const std::function<gid_t()> mock_getegid = [&]() { return desiredGroupId; };

        const std::function<struct group*(const char*)> mock_getgrnam = [](const char*) {
            static struct group desiredGroupEntry = {};
            desiredGroupEntry.gr_gid = desiredGroupId;

            return &desiredGroupEntry;
        };

        CHECK(VerifyProcessEffectiveGroup("desiredGroup" /* groupName */, mock_getegid, mock_getgrnam));
    }
}

TEST_CASE("VerifyProcessEffectiveUser")
{
    VECTOR_HANDLE empty_user_list = VECTOR_create(sizeof(STRING_HANDLE));
    STRING_HANDLE aduStr = STRING_construct("adu");
    STRING_HANDLE doStr = STRING_construct("do");
    VECTOR_HANDLE user_list = VECTOR_create(sizeof(STRING_HANDLE));
    VECTOR_push_back(user_list, &aduStr, 1);
    VECTOR_push_back(user_list, &doStr, 1);

    SECTION("it should return EPERM when there is no trusted user provided")
    {
        const std::function<uid_t()> mock_geteuid = [&]() { return 101; };

        const std::function<struct passwd*(const char*)> mock_getpwnam = [](const char*) { return nullptr; };

        CHECK(!VerifyProcessEffectiveUser(empty_user_list, mock_geteuid, mock_getpwnam));
    }

    SECTION("it should return EPERM when not root and not trusted user")
    {
        int effectiveProcessUserId = 100; // not root(0)

        const std::function<uid_t()> mock_geteuid = [&]() { return effectiveProcessUserId; };

        const std::function<struct passwd*(const char*)> mock_getpwnam = [effectiveProcessUserId](const char*) {
            static struct passwd trustedUserEntry = {};
            trustedUserEntry.pw_uid = effectiveProcessUserId + 1; // does not match effective process user id
            return &trustedUserEntry;
        };

        CHECK(!VerifyProcessEffectiveUser(user_list, mock_geteuid, mock_getpwnam));
    }

    SECTION("it should return 0(success) when root")
    {
        int effectiveProcessUserId = 0; // root
        const std::function<uid_t()> mock_geteuid = [&]() { return effectiveProcessUserId; };

        const std::function<struct passwd*(const char*)> mock_getpwnam = [](const char*) {
            static struct passwd trustedUserEntry = {};
            trustedUserEntry.pw_uid = 100; // not root

            return &trustedUserEntry;
        };

        CHECK(VerifyProcessEffectiveUser(user_list, mock_geteuid, mock_getpwnam));
    }

    SECTION("it should return 0(success) when the user is one of the trusted Users")
    {
        int effectiveProcessUserId = 100; // not root
        const std::function<uid_t()> mock_geteuid = [&]() { return effectiveProcessUserId; };

        const std::function<struct passwd*(const char*)> mock_getpwnam = [](const char*) {
            static struct passwd trustedUserEntry = {};
            trustedUserEntry.pw_uid = 100; // not root

            return &trustedUserEntry;
        };

        CHECK(VerifyProcessEffectiveUser(user_list, mock_geteuid, mock_getpwnam));
    }

    // Freeing user_list and empty_user_list
    for (size_t i = 0; i < VECTOR_size(user_list); i++)
    {
        auto user = static_cast<STRING_HANDLE*>(VECTOR_element(user_list, i));
        STRING_delete(*user);
    }

    VECTOR_clear(user_list);
    VECTOR_clear(empty_user_list);
}
