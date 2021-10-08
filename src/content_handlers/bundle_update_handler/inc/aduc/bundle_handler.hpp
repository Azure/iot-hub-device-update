/**
 * @file bundle_handler.hpp
 * @brief Defines BundleHandlerImpl.
 *
 * @copyright Copyright (c) Microsoft Corp.
 */
#ifndef ADUC_SIMULATOR_HANDLER_HPP
#define ADUC_SIMULATOR_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include "aduc/content_handler_factory.hpp"
#include <aduc/result.h>
#include <memory>
#include <string>

/**
 * @class BundleHandlerImpl
 * @brief The apt specific simulator implementation.
 */
class BundleHandlerImpl : public ContentHandler
{
public:
    static ContentHandler* CreateContentHandler();

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    BundleHandlerImpl(const BundleHandlerImpl&) = delete;
    BundleHandlerImpl& operator=(const BundleHandlerImpl&) = delete;
    BundleHandlerImpl(BundleHandlerImpl&&) = delete;
    BundleHandlerImpl& operator=(BundleHandlerImpl&&) = delete;

    ~BundleHandlerImpl() override = default;

    ADUC_Result Download(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override;

private:
    // Private constructor, must call CreateContentHandler factory method.
    BundleHandlerImpl()
    {
    }
};

#endif // ADUC_SIMULATOR_HANDLER_HPP
