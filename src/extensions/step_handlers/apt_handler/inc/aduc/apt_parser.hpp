/**
 * @file apt_parser.hpp
 * @brief Defines types and methods required to parse APT file (JSON format).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_APT_PARSER_HPP
#define ADUC_APT_PARSER_HPP

#include <aduc/result.h>
#include <list>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief JSON field name for packages property
 */
#define ADU_APT_FIELDNAME_PACKAGES "packages"

/**
 * @brief JSON field name for name property
 */
#define ADU_APT_FIELDNAME_NAME "name"

/**
 * @brief JSON field name for version property
 */
#define ADU_APT_FIELDNAME_VERSION "version"

/**
 * @brief JSON field name for agentRestartRequired property
 */
#define ADU_APT_FIELDNAME_AGENT_RESTART_REQUIRED "agentRestartRequired"

struct AptContent
{
    std::string Id;
    std::string Name;
    std::string Version;
    std::list<std::string> Packages;
    bool AgentRestartRequired;
};

namespace AptParser
{
/**
* @brief Parses APT content from specified file.
*
* @param filepath A full path to .json file contains APT data.
* @return std::unique_ptr<APTContent> APTContent object.
*/
std::unique_ptr<AptContent> ParseAptContentFromFile(const std::string& filepath);

/**
* @brief Parses content from json string.
*
* @param aptString A json string contains APT data.
* @return std::unique_ptr<APTContent> APTContent object.
*/
std::unique_ptr<AptContent> ParseAptContentFromString(const std::string& aptString);

/**
 * @class APT ParserException
 * @brief Exception throw by APT Parser.
 */
class ParserException : public std::exception
{
public:
    explicit ParserException(const std::string& message) : _message(message), _extendedResultCode(0)
    {
    }

    ParserException(const std::string& message, const int extendedResultCode) :
        _message(message), _extendedResultCode(extendedResultCode)
    {
    }

    const char* what() const noexcept override
    {
        return _message.c_str();
    }

    const int extendedResultCode()
    {
        return _extendedResultCode;
    }

private:
    std::string _message;
    int _extendedResultCode;
};

}; // namespace AptParser

#endif // ADUC_APT_PARSER_HPP
