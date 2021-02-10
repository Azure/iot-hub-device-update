/**
 * @file apt_handler.h
 * @brief Defines types and methods for APT handler plug-in for APT (Advanced Package Tool)
 *
 * @copyright Copyright (c) 2020, Microsoft Corp.
 */

#ifndef ADUC_APT_HANDLER_HPP
#define ADUC_APT_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include "aduc/content_handler_factory.hpp"
#include "apt_parser.hpp"
#include <aduc/result.h>

/**
 * @brief handler creation function
 * This function calls  CreateContentHandler from handler factory 
 */
std::unique_ptr<ContentHandler> microsoft_apt_CreateFunc(const ContentHandlerCreateData& data);

/**
 * @class AptHandler
 * @brief The APT handler implementation.
 */
class AptHandlerImpl : public ContentHandler
{
public:
    static std::unique_ptr<ContentHandler>
    CreateContentHandler(const std::string& workFolder, const std::string& filename);

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    AptHandlerImpl(const AptHandlerImpl&) = delete;
    AptHandlerImpl& operator=(const AptHandlerImpl&) = delete;
    AptHandlerImpl(AptHandlerImpl&&) = delete;
    AptHandlerImpl& operator=(AptHandlerImpl&&) = delete;

    ~AptHandlerImpl() override = default;

    ADUC_Result Prepare(const ADUC_PrepareInfo* prepareInfo) override;
    ADUC_Result Download() override;
    ADUC_Result Install() override;
    ADUC_Result Apply() override;
    ADUC_Result Cancel() override;
    ADUC_Result IsInstalled(const std::string& installedCriteria) override;

    static const ADUC_Result
    GetIsInstalled(const char* installedCriteriaFilePath, const std::string& installedCriteria);
    static const bool
    PersistInstalledCriteria(const char* installedCriteriaFilePath, const std::string& installedCriteria);
    static const bool
    RemoveInstalledCriteria(const char* installedCriteriaFilePath, const std::string& installedCriteria);
    static void RemoveAllInstalledCriteria();

protected:
    AptHandlerImpl(const std::string& workFolder, const std::string& filename);

private:
    std::unique_ptr<AptContent> _aptContent;

    bool _applied{ false };
    std::string _workFolder;
    std::string _logFolder;
    std::string _filename;
    std::list<std::string> _packages;

    void CreatePersistedId();
};

/**
 * @class APTHandlerException
 * @brief Exception throw by APTHandler and its components.
 */
class AptHandlerException : public std::exception
{
public:
    AptHandlerException(const std::string& message, const int extendedResultCode) :
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

#endif // ADUC_APT_HANDLER_HPP
