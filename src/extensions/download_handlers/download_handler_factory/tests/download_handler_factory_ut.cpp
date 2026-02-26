/**
 * @file download_handler_factory_ut.cpp
 * @brief Unit Tests for DownloadHandlerFactory.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

extern "C" {
#include "aduc/download_handler_factory.h"
}
#include "aduc/download_handler_factory.hpp"

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

// =====================================================================
// DownloadHandlerFactory singleton identity test
// =====================================================================

TEST_CASE("DownloadHandlerFactory GetInstance is stable across many calls")
{
    // Verify the singleton pointer is the same across a large number of calls
    DownloadHandlerFactory* first = DownloadHandlerFactory::GetInstance();
    for (int i = 0; i < 100; ++i)
    {
        CHECK(DownloadHandlerFactory::GetInstance() == first);
    }
}
