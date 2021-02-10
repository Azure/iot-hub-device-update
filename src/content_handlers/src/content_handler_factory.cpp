/**
 * @file content_handler_factory.cpp
 * @brief Implementation of ContentHandlerFactory.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include "aduc/content_handler_factory.hpp"
#include "aduc/content_handler.hpp"
#include "aduc/handler_function_map.hpp"

#include <aduc/c_utils.h>
#include <aduc/exceptions.hpp>
#include <aduc/logging.h>
#include <aduc/string_utils.hpp>
#include <cstring>
#include <vector>

/**
 * @brief Creates a ContentHandler
 *
 * @param type The type of the content handler to create.
 * @param data The data needed to create the content handler.
 *
 * @return ContentHandler The created content handler.
 */
std::unique_ptr<ContentHandler>
ContentHandlerFactory::Create(const char* updateType, const ContentHandlerCreateData& data)
{
    const std::string updateTypeStr(updateType);
    const std::vector<std::string> typeInfo = ADUC::StringUtils::Split(updateTypeStr, ':');
    if (typeInfo.size() == 2)
    {
        const std::string& updateTypeName = typeInfo[0];
        for (const auto& mapEntry : handlerCreateFuncs)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (updateTypeName == mapEntry.UpdateType) // provider/name matching is case sensitive
            {
                return mapEntry.CreateFunc(data);
            }
        }
    }
    else
    {
        Log_Error("Wrong format of update type :%s, expecting format - Provider/Name:Version", updateType);
    }

    ADUC::Exception::ThrowErrc(std::errc::operation_not_supported, "Unknown updateType");
    return nullptr;
}