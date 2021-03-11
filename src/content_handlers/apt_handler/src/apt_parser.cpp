/**
 * @file apt_parser.cpp
 * @brief Implements a parser for APT file.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 */
#include "aduc/apt_parser.hpp"
#include <aduc/logging.h>
#include <parson.h>
#include <string>

struct JSONValueDeleter
{
    void operator()(JSON_Value* value)
    {
        json_value_free(value);
    }
};

using AutoFreeJsonValue_t = std::unique_ptr<JSON_Value, JSONValueDeleter>;

/**
 * @brief Parse json root value and create AptConent.
 * @details Example of APT Content RootValue
 *
 *{
 *   "name" : "contoso-smart-thomostats",
 *   "version" : "1.0.1",
 *   "packages" : [
 *                   {
 *                       "name" : "thermocontrol",
 *                       "version" : "1.0.1"
 *                   },
 *                   {
 *                       "name" : "tempreport",
 *                       "version" : "2.0.0"
 *                   }
 *               ]
 * }
 * @param rootValue
 * @return std::unique_ptr<AptContent>.
 */
std::unique_ptr<AptContent> GetAptContentFromRootValue(JSON_Value* rootValue)
{
    JSON_Object* rootObject = json_value_get_object(rootValue);
    if (rootObject == nullptr)
    {
        throw std::bad_alloc();
    }

    const char* aptName = json_object_get_string(rootObject, ADU_APT_FIELDNAME_NAME);
    const char* aptVersion = json_object_get_string(rootObject, ADU_APT_FIELDNAME_VERSION);

    if (aptName == nullptr)
    {
        throw AptParser::ParserException("Missing APT name.");
    }

    if (aptVersion == nullptr)
    {
        throw AptParser::ParserException("Missing APT version.");
    }

    // NOLINTNEXTLINE(modernize-make-unique)
    std::unique_ptr<AptContent> aptContent = std::unique_ptr<AptContent>(new AptContent());
    aptContent->Name = aptName;
    aptContent->Version = aptVersion;

    // Note: json_object_get_boolean returns -1 on fail.
    const int agentRestartRequired = json_object_get_boolean(rootObject, ADU_APT_FIELDNAME_AGENT_RESTART_REQUIRED);
    aptContent->AgentRestartRequired = (agentRestartRequired > 0);

    // Parse package list.
    JSON_Array* packages = json_object_get_array(rootObject, ADU_APT_FIELDNAME_PACKAGES);
    if (packages != nullptr)
    {
        if (json_array_get_count(packages) == 0)
        {
            throw AptParser::ParserException(
                "APT Handler configuration data doesn't contain packages.", ADUC_ERC_APT_HANDLER_INVALID_PACKAGE_DATA);
        }

        for (size_t i = 0; i < json_array_get_count(packages); i++)
        {
            JSON_Object* package = json_array_get_object(packages, i);
            std::string name = json_object_get_string(package, "name");
            if (name.empty())
            {
                throw AptParser::ParserException(
                    "APT Handler configuration data contains empty package name.",
                    ADUC_ERC_APT_HANDLER_INVALID_PACKAGE_DATA);
            }

            // Are we installing deviceupdate-agent package?
            // Currently, we are assuming deviceupdate-agent* is a Device Update Agent package.
            if (!aptContent->AgentRestartRequired && (name.find("deviceupdate-agent") == 0))
            {
                aptContent->AgentRestartRequired = true;
                Log_Info(
                    "The DU Agent restart is required after installation task completed. (package:%s)", name.c_str());
            }

            // NOTE: Version is optional.
            // We don't do any parsing of the package version string here.
            // Customer need to specify exact string that match the version they want.
            // If we can't find a package with specified version, we'll fail during download.
            const char* version = json_object_get_string(package, "version");
            if (version != nullptr)
            {
                name += "=";
                name += version;
            }

            aptContent->Packages.emplace_back(name);
        }
    }

    return aptContent;
}

std::unique_ptr<AptContent> AptParser::ParseAptContentFromFile(const std::string& filepath)
{
    AutoFreeJsonValue_t rootValue{ json_parse_file(filepath.c_str()) };
    if (!rootValue)
    {
        Log_Error("Failed to parse specified APT file (%s).", filepath.c_str());
        throw AptParser::ParserException("Cannot parse specified APT file.");
    }
    return GetAptContentFromRootValue(rootValue.get());
}

std::unique_ptr<AptContent> AptParser::ParseAptContentFromString(const std::string& aptString)
{
    AutoFreeJsonValue_t rootValue{ json_parse_string(aptString.c_str()) };
    if (!rootValue)
    {
        Log_Error("Failed to parse specified APT string content");
        throw AptParser::ParserException("Cannot parse specified APT string.");
    }

    return GetAptContentFromRootValue(rootValue.get());
}
