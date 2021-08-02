/**
 * @file pvcontrol_handler.hpp
 * @brief Defines PVControlHandlerImpl.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 * @copyright Copyright (c) 2021, Pantacor Ltd.
 */
#ifndef ADUC_PVCONTROL_HANDLER_HPP
#define ADUC_PVCONTROL_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include "aduc/content_handler_factory.hpp"
#include <aduc/result.h>
#include <memory>
#include <string>

/**
 * @brief handler creation function
 * This function calls  CreateContentHandler from handler factory 
 */
std::unique_ptr<ContentHandler> pantacor_pvcontrol_CreateFunc(const ContentHandlerCreateData& data);

/**
 * @class PVControlHandlerImpl
 * @brief The pvcontrol specific implementation of ContentHandler interface.
 */
class PVControlHandlerImpl : public ContentHandler
{
public:
    static std::unique_ptr<ContentHandler>
    CreateContentHandler(const std::string& workFolder, const std::string& logFolder, const std::string& filename);

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    PVControlHandlerImpl(const PVControlHandlerImpl&) = delete;
    PVControlHandlerImpl& operator=(const PVControlHandlerImpl&) = delete;
    PVControlHandlerImpl(PVControlHandlerImpl&&) = delete;
    PVControlHandlerImpl& operator=(PVControlHandlerImpl&&) = delete;

    ~PVControlHandlerImpl() override = default;

    ADUC_Result Prepare(const ADUC_PrepareInfo* prepareInfo) override;
    ADUC_Result Download() override;
    ADUC_Result Install() override;
    ADUC_Result Apply() override;
    ADUC_Result Cancel() override;
    ADUC_Result IsInstalled(const std::string& installedCriteria) override;

    static std::string ReadValueFromFile(const std::string& filePath);

protected:
    // Protected constructor, must call CreateContentHandler factory method or from derived simulator class
    PVControlHandlerImpl(const std::string& workFolder, const std::string& logFolder, const std::string& filename) :
        _workFolder{ workFolder }, _logFolder{ logFolder }, _filename{ filename }
    {
    }

private:
    std::string _workFolder;
    std::string _logFolder;
    std::string _filename;
    bool _isApply{ false };
};

#endif // ADUC_PVCONTROL_HANDLER_HPP
