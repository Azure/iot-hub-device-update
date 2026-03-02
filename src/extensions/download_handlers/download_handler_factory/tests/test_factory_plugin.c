/**
 * @file test_factory_plugin.c
 * @brief Minimal test download handler plugin for factory unit tests.
 *
 * Exports only Initialize and Cleanup so the factory can successfully
 * create and destroy a DownloadHandlerPlugin instance.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

void Initialize(int logLevel)
{
    (void)logLevel;
}

void Cleanup(void)
{
}
