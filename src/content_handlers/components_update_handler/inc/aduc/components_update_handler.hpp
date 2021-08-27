/**
 * @file components_update_handler.hpp
 * @brief Defines ComponentsUpdateHandlerImpl.
 *
 * @copyright Copyright (c) Microsoft Corp.
 */
#ifndef ADUC_COMPONENTS_UPDATE_HANDLER_HPP
#define ADUC_COMPONENTS_UPDATE_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include "aduc/content_handler_factory.hpp"
#include <aduc/result.h>
#include <memory>
#include <string>

/**
 * @class ComponentsUpdateHandlerImpl
 * @brief The apt specific simulator implementation.
 */
class ComponentsUpdateHandlerImpl : public ContentHandler
{
public:
    static ContentHandler* CreateContentHandler();

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    ComponentsUpdateHandlerImpl(const ComponentsUpdateHandlerImpl&) = delete;
    ComponentsUpdateHandlerImpl& operator=(const ComponentsUpdateHandlerImpl&) = delete;
    ComponentsUpdateHandlerImpl(ComponentsUpdateHandlerImpl&&) = delete;
    ComponentsUpdateHandlerImpl& operator=(ComponentsUpdateHandlerImpl&&) = delete;

    ~ComponentsUpdateHandlerImpl() override = default;

    ADUC_Result Download(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override;

    void SetIsInstalled(bool installed);

private:
    ComponentsUpdateHandlerImpl()
    {
    }
};

#endif // ADUC_COMPONENTS_UPDATE_HANDLER_HPP
