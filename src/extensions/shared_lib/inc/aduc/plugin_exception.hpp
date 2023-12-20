
/**
 * @file plugin_exception.hpp
 * @brief header for aduc plug exception thrown for issues with plugin/shared lib.
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <stdexcept>
#include <string>

namespace aduc
{
class PluginException : public std::runtime_error
{
    std::string export_symbol;

public:
    /**
     * @brief default constructor for PluginException
     * @param msg message to set the runtime_error to
     * @param symbol the string representation of the exception's symbol
     */
    PluginException(const std::string& msg, const std::string symbol) :
        std::runtime_error{ msg }, export_symbol{ symbol }
    {
    }

    /**
     * @brief Getter for the export_symbol member variable
     * @returns the string representation of the export_symbol
     */
    const std::string& Symbol() const
    {
        return export_symbol;
    }
};

} // namespace aduc
