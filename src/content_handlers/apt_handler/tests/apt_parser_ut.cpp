/**
 * @file apt_parser_ut.cpp
 * @brief APT parser unit tests
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/apt_parser.hpp"

#include <catch2/catch.hpp>
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
