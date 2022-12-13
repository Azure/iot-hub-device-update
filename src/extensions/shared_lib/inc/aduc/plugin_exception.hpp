
/**
 * @file aduc_plugin_exception.hpp
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
    PluginException(const std::string& msg, const std::string symbol) :
        std::runtime_error{ msg }, export_symbol{ symbol }
    {
    }

    const std::string& Symbol() const
    {
        return export_symbol;
    }
};

} // namespace aduc
