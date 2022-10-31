/**
 * @file simulator_handler.hpp
 * @brief Defines SimulatorHandlerImpl.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_SIMULATOR_HANDLER_HPP
#define ADUC_SIMULATOR_HANDLER_HPP

#include "aduc/content_handler.hpp"
#include "aduc/logging.h"

EXTERN_C_BEGIN

/**
 * @brief Instantiates an Update Content Handler simulator.
 * @return A pointer to an instantiated Update Content Handler object.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);

EXTERN_C_END

/**
 * @class SimulatorHandlerImpl
 * @brief The simulator handler implementation.
 */
class SimulatorHandlerImpl : public ContentHandler
{
public:
    static ContentHandler* CreateContentHandler();

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    SimulatorHandlerImpl(const SimulatorHandlerImpl&) = delete;
    SimulatorHandlerImpl& operator=(const SimulatorHandlerImpl&) = delete;
    SimulatorHandlerImpl(SimulatorHandlerImpl&&) = delete;
    SimulatorHandlerImpl& operator=(SimulatorHandlerImpl&&) = delete;

    ~SimulatorHandlerImpl() override;

    ADUC_Result Download(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Backup(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Install(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Apply(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Restore(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result Cancel(const tagADUC_WorkflowData* workflowData) override;
    ADUC_Result IsInstalled(const tagADUC_WorkflowData* workflowData) override;

private:
    // Private constructor, must call CreateContentHandler factory method.
    SimulatorHandlerImpl()
    {
    }
};

/**
 * @brief Get the simulator data file path.
 *
 * @return char* A buffer contains file path. Caller must call free() once done.
 */
char* GetSimulatorDataFilePath();

#endif // ADUC_SIMULATOR_HANDLER_HPP
