/**
 * @file viewstatemgr_unit_tests.cpp
 * @brief Unit Tests for viewstatemgr library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/aducsdk.h" // ADUC_ServiceStatus
#include "aduc/defer.hpp"
#include "aduc/result.h"
#include "aduc/viewstatemgr.h"

#include <catch2/catch_all.hpp>
using Catch::Matchers::Equals;

TEST_CASE("viewstatemgr get set")
{
    ViewStateManager m = { 0 };
    REQUIRE(0 == viewstatemgr_create(&m));
    aduc::Defer defer_destroy([&]() { viewstatemgr_destroy(&m); });
    ADUC_ServiceStatus status = ADUC_ServiceStatus_None;
    REQUIRE(viewstatemgr_svcstatus_get(&m, &status));
    CHECK(status == ADUC_ServiceStatus_Initializing);
    viewstatemgr_svcstatus_set(&m, ADUC_ServiceStatus_Installing);
    CHECK(viewstatemgr_svcstatus_get(&m, &status));
    CHECK(status == ADUC_ServiceStatus_Installing);
}
