/**
 * @file test_factory_noinit_plugin.c
 * @brief Test plugin deliberately missing the Initialize export.
 *
 * When DownloadHandlerPlugin tries to call Initialize via CallExport,
 * the symbol lookup will fail and throw aduc::PluginException.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

void Cleanup(void)
{
}

void Placeholder(void)
{
}
