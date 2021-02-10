/**
 * @file content_handler.hpp
 * @brief Defines ContentHandler interface.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_CONTENT_HANDLER_HPP
#define ADUC_CONTENT_HANDLER_HPP

#include <string>

#include <aduc/adu_core_exports.h>
#include <aduc/result.h>

/**
 * @interface ContentHandler
 * @brief Interface for content specific handler implementations.
 */
class ContentHandler
{
public:
    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    ContentHandler(const ContentHandler&) = delete;
    ContentHandler& operator=(const ContentHandler&) = delete;
    ContentHandler(ContentHandler&&) = delete;
    ContentHandler& operator=(ContentHandler&&) = delete;

    virtual ~ContentHandler() = default;

    virtual ADUC_Result Prepare(const ADUC_PrepareInfo* prepareInfo) = 0;
    virtual ADUC_Result Download() = 0;
    virtual ADUC_Result Install() = 0;
    virtual ADUC_Result Apply() = 0;
    virtual ADUC_Result Cancel() = 0;
    virtual ADUC_Result IsInstalled(const std::string& installedCriteria) = 0;

protected:
    ContentHandler() = default;
};

#endif // ADUC_CONTENT_HANDLER_HPP
