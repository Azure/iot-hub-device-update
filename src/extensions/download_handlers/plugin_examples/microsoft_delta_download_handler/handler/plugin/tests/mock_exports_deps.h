/**
 * @file mock_exports_deps.h
 * @brief Configurable mock state for EXPORTS.c dependencies.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef MOCK_EXPORTS_DEPS_H
#define MOCK_EXPORTS_DEPS_H

#include <aduc/result.h>

struct MockExportsDepsState
{
    /** Result returned by MicrosoftDeltaDownloadHandler_ProcessUpdate */
    ADUC_Result processUpdateResult;
    int processUpdateCallCount;

    /** Result returned by MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted */
    ADUC_Result onUpdateWorkflowCompletedResult;
    int onUpdateWorkflowCompletedCallCount;

    /** Track logging init/uninit calls */
    int loggingInitCallCount;
    int loggingUninitCallCount;
    int lastLogLevel;
};

extern MockExportsDepsState g_mockExportsDeps;

void ResetExportsMocks(void);

#endif /* MOCK_EXPORTS_DEPS_H */
