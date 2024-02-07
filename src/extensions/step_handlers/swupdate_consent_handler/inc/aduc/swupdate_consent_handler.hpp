/**
 * @file swupdate_consent_handler.hpp
 * @brief Defines SWUpdateConsentHandlerImpl.
 *
 * @copyright Copyright (c) conplement AG.
 * Licensed under either of Apache-2.0 or MIT license at your option.
 */
#ifndef ADUC_SWUPDATECONSENT_HANDLER_HPP
#define ADUC_SWUPDATECONSENT_HANDLER_HPP

#include <string>

#include "aduc/content_handler.hpp"
#include "aduc/logging.h"
#include <parson.h>


EXTERN_C_BEGIN

/**
 * @brief Instantiates an Update Content Handler swupdateconsent.
 * @return A pointer to an instantiated Update Content Handler object.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);

EXTERN_C_END

/**
 * @class SWUpdateConsentHandlerImpl
 * @brief The swupdateconsent handler implementation.
 */
class SWUpdateConsentHandlerImpl : public ContentHandler
{
public:
    static ContentHandler* CreateContentHandler();

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    SWUpdateConsentHandlerImpl(const SWUpdateConsentHandlerImpl&) = delete;
    SWUpdateConsentHandlerImpl& operator=(const SWUpdateConsentHandlerImpl&) = delete;
    SWUpdateConsentHandlerImpl(SWUpdateConsentHandlerImpl&&) = delete;
    SWUpdateConsentHandlerImpl& operator=(SWUpdateConsentHandlerImpl&&) = delete;

    ~SWUpdateConsentHandlerImpl() override;

    ADUC_Result Download(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Backup(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Restore(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const tagADUC_WorkflowData* workflowData) override;

private:
    const std::string generalConsent = "swupdate";

    // Private constructor, must call CreateContentHandler factory method.
    SWUpdateConsentHandlerImpl()
    {
    }
    std::string ValueOrEmpty(const char* s);
    std::string ReadValueFromFile(const std::string& filePath);
    JSON_Object* ReadJsonDataFile(const std::string& filePath);
    bool GetGeneralConsent(void);
    ADUC_GeneralResult CleanUserConsentAgreed(void);
    bool UserConsentAgreed(const std::string& version);
    bool AppendArrayRecord(const JSON_Object* root, const char* arrayName, JSON_Value* record);
    ADUC_GeneralResult UpdateConsentRequestJsonFile(const std::string& version);
    ADUC_GeneralResult CleanConsentRequestJsonFile(void);
    ADUC_GeneralResult UpdateConsentHistoryJsonFile(const std::string& version);
    std::string GetVersion(const std::string& installedCriteria);
};

#endif // ADUC_SWUPDATECONSENT_HANDLER_HPP
