/**
 * @file viewstatemgr_unit_tests.cpp
 * @brief Unit Tests for viewstatemgr library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/viewstatemgr.h"
#include "aduc/result.h"
#include "aduc/defer.hpp"
#include "aduc/aducsdk.h" // ADUC_ServiceStatus

#include <catch2/catch_all.hpp>
using Catch::Matchers::Equals;

TEST_CASE("viewstatemgr get set")
{
    ViewStateMgrHandle handle = viewstatemgr_create();
    aduc::Defer defer_destroy([handle]() { viewstatemgr_destroy(handle); });
    ADUC_ServiceStatus status = ADUC_ServiceStatus_None;
    ADUC_Result result = viewstatemgr_svcstatus_get(handle, &status);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(IsAducResultCodeSuccess(viewstatemgr_svcstatus_set(handle, ADUC_ServiceStatus_Installing).ResultCode));
    REQUIRE(IsAducResultCodeSuccess(viewstatemgr_svcstatus_get(handle, &status).ResultCode));
    CHECK(status == ADUC_ServiceStatus_Installing);
}
