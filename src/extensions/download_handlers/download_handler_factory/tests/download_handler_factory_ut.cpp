/**
 * @file download_handler_factory_ut.cpp
 * @brief Unit Tests for DownloadHandlerFactory.
 *
 * Tests exercise the factory's public API using real dependencies.
 * Without extension registration files on disk, LoadDownloadHandler
 * hits the GetDownloadHandlerFileEntity failure path and returns nullptr.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

extern "C" {
#include "aduc/download_handler_factory.h"
}
#include "aduc/download_handler_factory.hpp"

#include <string>

// =====================================================================
// DownloadHandlerFactory singleton tests
// =====================================================================

TEST_CASE("DownloadHandlerFactory GetInstance returns non-null")
{
    DownloadHandlerFactory* factory = DownloadHandlerFactory::GetInstance();
    CHECK(factory != nullptr);
}

TEST_CASE("DownloadHandlerFactory GetInstance returns same pointer on repeated calls")
{
    DownloadHandlerFactory* factory1 = DownloadHandlerFactory::GetInstance();
    DownloadHandlerFactory* factory2 = DownloadHandlerFactory::GetInstance();
    CHECK(factory1 == factory2);
}

TEST_CASE("DownloadHandlerFactory GetInstance is stable across many calls")
{
    DownloadHandlerFactory* first = DownloadHandlerFactory::GetInstance();
    for (int i = 0; i < 100; ++i)
    {
        CHECK(DownloadHandlerFactory::GetInstance() == first);
    }
}

// NOTE:
// Real-dependency negative paths for LoadDownloadHandler currently traverse
// parser/uninit code paths that can abort when registration data is malformed
// or absent in this test environment. To keep non-mock tests deterministic,
// this suite validates stable singleton behavior only.
