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

EXTERN_C_BEGIN

/**
 * @brief Instantiates an Update Content Handler for 'microsoft/swupdate:1' update type.
 * @return A pointer to an instantiated Update Content Handler object.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);

EXTERN_C_END

/**
 * @class SWUpdateHandlerImpl
 * @brief The swupdate specific implementation of ContentHandler interface.
 */
class SWUpdateHandlerImpl : public ContentHandler
{
public:
    static ContentHandler* CreateContentHandler();

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    SWUpdateHandlerImpl(const SWUpdateHandlerImpl&) = delete;
    SWUpdateHandlerImpl& operator=(const SWUpdateHandlerImpl&) = delete;
    SWUpdateHandlerImpl(SWUpdateHandlerImpl&&) = delete;
    SWUpdateHandlerImpl& operator=(SWUpdateHandlerImpl&&) = delete;

    ~SWUpdateHandlerImpl() override = default;

    ADUC_Result Download(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override;

    static std::string ReadValueFromFile(const std::string& filePath);

protected:
    // Protected constructor, must call CreateContentHandler factory method or from derived simulator class
    SWUpdateHandlerImpl()
    {
    }
};

#endif // ADUC_SWUPDATE_HANDLER_HPP
