/**
 * @file apt_parser_ut.cpp
 * @brief APT parser unit tests
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/apt_parser.hpp"

#include <catch2/catch_all.hpp>
using Catch::Matchers::Equals;

#include <sstream>
#include <string>

/* Example of a APT Json
{
    "name":"com-microsoft-eds-adu-testapt",
    "version":"1.0.0",
    "packages": [
                    {
                        "name":"moby-engine",
                        "version":"1.0.0.0"
                    },
                    {
                        "name":"iotedge",
                        "version":"2.0.0.0"
                    }
                ]
}
*/

// clang-format off

const char* aptContentWithThreePackages =
    R"({)"
        R"( "name":"com-microsoft-eds-adu-testapt", )"
        R"( "version":"1.0.0", )"
        R"( "packages": [ )"
            R"({)"
                R"( "name":"moby-engine", )"
                R"( "version":"1.0.0.0" )"
            R"(},)"
            R"({)"
                R"( "name":"libiothsm-std" )"
            R"(},)"
            R"({)"
                R"( "name":"iotedge", )"
                R"( "version":"2.0.0.0" )"
            R"(})"
            R"(])"
    R"(})";


const char* aptContentWithOnePackage =
    R"({)"
        R"( "name":"com-microsoft-eds-adu-testapt", )"
        R"( "version":"1.0.1", )"
        R"( "packages": [ )"
            R"({)"
                R"( "name":"moby-engine", )"
                R"( "version":"1.0.0.0" )"
            R"(})"
            R"(])"
    R"(})";

const char* aptContentWithAgentRestartRequired =
    R"({)"
        R"( "name":"com-microsoft-eds-adu-testapt", )"
        R"( "version":"1.0.1", )"
        R"( "agentRestartRequired":true, )"
        R"( "packages": [ )"
            R"({)"
                R"( "name":"moby-engine", )"
                R"( "version":"1.0.0.0" )"
            R"(})"
            R"(])"
    R"(})";

const char* aptContentWithAgentRestartRequiredFalse =
    R"({)"
        R"( "name":"com-microsoft-eds-adu-testapt", )"
        R"( "version":"1.0.1", )"
        R"( "agentRestartRequired":false, )"
        R"( "packages": [ )"
            R"({)"
                R"( "name":"moby-engine", )"
                R"( "version":"1.0.0.0" )"
            R"(})"
            R"(])"
    R"(})";

const char* aptContentWithAgentRestartRequiredUsingNameDuAgent =
    R"({)"
        R"( "name":"com-microsoft-eds-adu-testapt", )"
        R"( "version":"1.0.1", )"
        R"( "packages": [ )"
            R"({)"
                R"( "name":"deviceupdate-agent" )"
            R"(})"
            R"(])"
    R"(})";

// clang-format on

TEST_CASE("APT Parser Tests")
{
    std::unique_ptr<AptContent> aptContent = AptParser::ParseAptContentFromString(aptContentWithThreePackages);
    CHECK_THAT(aptContent->Name, Equals("com-microsoft-eds-adu-testapt"));
    CHECK_THAT(aptContent->Version, Equals("1.0.0"));
    const std::list<std::string> expectedPkgs{ "moby-engine=1.0.0.0", "libiothsm-std", "iotedge=2.0.0.0" };
    REQUIRE(aptContent->Packages == expectedPkgs);
}

TEST_CASE("APT Parser Tests 2")
{
    std::unique_ptr<AptContent> aptContent = AptParser::ParseAptContentFromString(aptContentWithOnePackage);
    CHECK_THAT(aptContent->Name, Equals("com-microsoft-eds-adu-testapt"));
    CHECK_THAT(aptContent->Version, Equals("1.0.1"));
    CHECK(aptContent->Packages.size() == 1);
    const auto package = aptContent->Packages.front();
    CHECK_THAT(package, Equals("moby-engine=1.0.0.0"));
    CHECK_FALSE(aptContent->AgentRestartRequired);
}

TEST_CASE("APT Parser AgentRestartRequired Test")
{
    std::unique_ptr<AptContent> aptContent = AptParser::ParseAptContentFromString(aptContentWithAgentRestartRequired);
    CHECK(aptContent->AgentRestartRequired);
}

TEST_CASE("APT Parser AgentRestartRequired(false) Test")
{
    std::unique_ptr<AptContent> aptContent =
        AptParser::ParseAptContentFromString(aptContentWithAgentRestartRequiredFalse);
    CHECK_FALSE(aptContent->AgentRestartRequired);
}

TEST_CASE("APT Parser AgentRestartRequired(du-agent package name) Test")
{
    std::unique_ptr<AptContent> aptContent =
        AptParser::ParseAptContentFromString(aptContentWithAgentRestartRequiredUsingNameDuAgent);
    CHECK(aptContent->AgentRestartRequired);
}

// =====================================================================
// Error-path tests for apt_parser.cpp
// =====================================================================

TEST_CASE("APT Parser: invalid JSON string throws ParserException")
{
    CHECK_THROWS_AS(AptParser::ParseAptContentFromString("not valid json"), AptParser::ParserException);
}

TEST_CASE("APT Parser: missing name throws ParserException")
{
    // clang-format off
    const char* json =
        R"({"version":"1.0","packages":[{"name":"pkg"}]})";
    // clang-format on
    CHECK_THROWS_AS(AptParser::ParseAptContentFromString(json), AptParser::ParserException);
}

TEST_CASE("APT Parser: missing version throws ParserException")
{
    // clang-format off
    const char* json =
        R"({"name":"test","packages":[{"name":"pkg"}]})";
    // clang-format on
    CHECK_THROWS_AS(AptParser::ParseAptContentFromString(json), AptParser::ParserException);
}

TEST_CASE("APT Parser: empty packages array throws ParserException")
{
    // clang-format off
    const char* json =
        R"({"name":"test","version":"1.0","packages":[]})";
    // clang-format on
    CHECK_THROWS_AS(AptParser::ParseAptContentFromString(json), AptParser::ParserException);
}

TEST_CASE("APT Parser: empty package name throws ParserException")
{
    // clang-format off
    const char* json =
        R"({"name":"test","version":"1.0","packages":[{"name":""}]})";
    // clang-format on
    CHECK_THROWS_AS(AptParser::ParseAptContentFromString(json), AptParser::ParserException);
}

TEST_CASE("APT Parser: no packages array -> empty packages list")
{
    // clang-format off
    const char* json =
        R"({"name":"test","version":"1.0"})";
    // clang-format on
    auto content = AptParser::ParseAptContentFromString(json);
    CHECK(content != nullptr);
    CHECK_THAT(content->Name, Equals("test"));
    CHECK_THAT(content->Version, Equals("1.0"));
    CHECK(content->Packages.empty());
}

TEST_CASE("APT Parser: non-existent file throws ParserException")
{
    CHECK_THROWS_AS(AptParser::ParseAptContentFromFile("/nonexistent/path.json"), AptParser::ParserException);
}

TEST_CASE("APT Parser: package with version appends =version")
{
    // clang-format off
    const char* json =
        R"({"name":"test","version":"1.0","packages":[{"name":"pkg","version":"2.0"}]})";
    // clang-format on
    auto content = AptParser::ParseAptContentFromString(json);
    REQUIRE(content->Packages.size() == 1);
    CHECK(content->Packages.front() == "pkg=2.0");
}

// =====================================================================
// ParserException tests (apt_parser.hpp coverage)
// =====================================================================

TEST_CASE("ParserException: single-arg constructor")
{
    AptParser::ParserException ex("test message");
    CHECK(std::string(ex.what()) == "test message");
    CHECK(ex.extendedResultCode() == 0);
}

TEST_CASE("ParserException: two-arg constructor with ERC")
{
    AptParser::ParserException ex("err msg", 42);
    CHECK(std::string(ex.what()) == "err msg");
    CHECK(ex.extendedResultCode() == 42);
}

// =====================================================================
// AptHandlerException tests (apt_handler.hpp coverage)
// =====================================================================

#include "aduc/apt_handler.hpp"

TEST_CASE("AptHandlerException: constructor, what, extendedResultCode")
{
    AptHandlerException ex("handler error", 123);
    CHECK(std::string(ex.what()) == "handler error");
    CHECK(ex.extendedResultCode() == 123);
}
