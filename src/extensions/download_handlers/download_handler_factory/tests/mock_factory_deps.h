/**
 * @file mock_factory_deps.h
 * @brief Configurable link-time mock state for download_handler_factory dependencies.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef MOCK_FACTORY_DEPS_H
#define MOCK_FACTORY_DEPS_H

struct MockFactoryDepsState
{
    bool getFileEntityResult;
    const char* fileEntityPath;
    bool verifyHashResult;
};

extern MockFactoryDepsState g_mockFactoryDeps;

#endif // MOCK_FACTORY_DEPS_H
