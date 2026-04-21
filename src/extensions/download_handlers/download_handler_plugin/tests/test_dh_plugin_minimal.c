/**
 * @file test_dh_plugin_minimal.c
 * @brief Minimal test plugin that only exports Initialize.
 *
 * Missing Cleanup, ProcessUpdate, OnUpdateWorkflowCompleted, GetContractInfo.
 * When DownloadHandlerPlugin tries to call these via CallExport, the symbol
 * lookup will fail and throw aduc::PluginException. This exercises the
 * PluginException catch blocks in each method.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

void Initialize(int logLevel)
{
    (void)logLevel;
}
