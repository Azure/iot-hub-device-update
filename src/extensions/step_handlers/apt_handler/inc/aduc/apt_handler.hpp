/**
 * @file apt_handler.h
 * @brief Defines types and methods for APT handler plug-in for APT (Advanced Package Tool)
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_APT_HANDLER_HPP
#define ADUC_APT_HANDLER_HPP

#include "aduc/apt_handler.hpp"
#include "aduc/content_handler.hpp"
#include "aduc/logging.h" // ADUC_LOG_SEVERITY
#include "aduc/result.h"
#include "apt_parser.hpp"

EXTERN_C_BEGIN

/**
 * @brief Instantiates an Update Content Handler for 'microsoft/apt:1' update type.
 * @return A pointer to an instantiated Update Content Handler object.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);

/**
 * @brief Gets the extension contract info.
 *
 * @param[out] contractInfo The extension contract info.
 * @return ADUC_Result The result.
 */
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);

EXTERN_C_END

/**
 * @class AptHandler
 * @brief The APT handler implementation.
 */
class AptHandlerImpl : public ContentHandler
{
public:
    static ContentHandler* CreateContentHandler();

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    AptHandlerImpl(const AptHandlerImpl&) = delete;
    AptHandlerImpl& operator=(const AptHandlerImpl&) = delete;
    AptHandlerImpl(AptHandlerImpl&&) = delete;
    AptHandlerImpl& operator=(AptHandlerImpl&&) = delete;

    ~AptHandlerImpl() override;

    ADUC_Result Download(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Backup(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Restore(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const tagADUC_WorkflowData* workflowData) override;

protected:
    AptHandlerImpl()
    {
    }

private:
    ADUC_Result ParseContent(const std::string& aptManifestFile, std::unique_ptr<AptContent>& aptContent);
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
