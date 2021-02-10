/**
 * @file swupdate_handler.hpp
 * @brief Defines SWUpdateHandlerImpl.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_SWUPDATE_HANDLER_HPP
#define ADUC_SWUPDATE_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include "aduc/content_handler_factory.hpp"
#include <aduc/result.h>
#include <memory>
#include <string>

/**
 * @brief handler creation function
 * This function calls  CreateContentHandler from handler factory 
 */
std::unique_ptr<ContentHandler> microsoft_swupdate_CreateFunc(const ContentHandlerCreateData& data);

/**
 * @class SWUpdateHandlerImpl
 * @brief The swupdate specific implementation of ContentHandler interface.
 */
class SWUpdateHandlerImpl : public ContentHandler
{
public:
    static std::unique_ptr<ContentHandler>
    CreateContentHandler(const std::string& workFolder, const std::string& logFolder, const std::string& filename);

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    SWUpdateHandlerImpl(const SWUpdateHandlerImpl&) = delete;
    SWUpdateHandlerImpl& operator=(const SWUpdateHandlerImpl&) = delete;
    SWUpdateHandlerImpl(SWUpdateHandlerImpl&&) = delete;
    SWUpdateHandlerImpl& operator=(SWUpdateHandlerImpl&&) = delete;

    ~SWUpdateHandlerImpl() override = default;

    ADUC_Result Prepare(const ADUC_PrepareInfo* prepareInfo) override;
    ADUC_Result Download() override;
    ADUC_Result Install() override;
    ADUC_Result Apply() override;
    ADUC_Result Cancel() override;
    ADUC_Result IsInstalled(const std::string& installedCriteria) override;

    static std::string ReadValueFromFile(const std::string& filePath);

protected:
    // Protected constructor, must call CreateContentHandler factory method or from derived simulator class
    SWUpdateHandlerImpl(const std::string& workFolder, const std::string& logFolder, const std::string& filename) :
        _workFolder{ workFolder }, _logFolder{ logFolder }, _filename{ filename }
    {
    }

private:
    std::string _workFolder;
    std::string _logFolder;
    std::string _filename;
    bool _isApply{ false };
};

#endif // ADUC_SWUPDATE_HANDLER_HPP
