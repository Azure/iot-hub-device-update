/**
 * @file adu_types_ut.cpp
 * @brief Unit Tests for adu_types library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
using Catch::Matchers::Equals;

#include <cstdlib>
#include <cstring>

#include "aduc/adu_types.h"
#include "aduc/types/update_content.h"

// =====================================================================
// ADUC_ConnType_ToString tests
// =====================================================================

TEST_CASE("ADUC_ConnType_ToString", "[adu_types]")
{
    SECTION("Returns correct string for ADUC_ConnType_NotSet")
    {
        CHECK_THAT(ADUC_ConnType_ToString(ADUC_ConnType_NotSet), Equals("ADUC_ConnType_NotSet"));
    }

    SECTION("Returns correct string for ADUC_ConnType_Device")
    {
        CHECK_THAT(ADUC_ConnType_ToString(ADUC_ConnType_Device), Equals("ADUC_ConnType_Device"));
    }

    SECTION("Returns correct string for ADUC_ConnType_Module")
    {
        CHECK_THAT(ADUC_ConnType_ToString(ADUC_ConnType_Module), Equals("ADUC_ConnType_Module"));
    }

    SECTION("Returns '<Unknown>' for an out-of-range value")
    {
        CHECK_THAT(ADUC_ConnType_ToString(static_cast<ADUC_ConnType>(99)), Equals("<Unknown>"));
    }
}

// =====================================================================
// ADUC_ConnectionInfo_DeAlloc tests
// =====================================================================

TEST_CASE("ADUC_ConnectionInfo_DeAlloc", "[adu_types]")
{
    SECTION("Deallocates all fields and resets sentinels")
    {
        ADUC_ConnectionInfo info = {};

        // Allocate heap strings to simulate real usage.
        info.connectionString = strdup("HostName=test.azure-devices.net");
        info.certificateString = strdup("-----BEGIN CERTIFICATE-----");
        info.opensslEngine = strdup("pkcs11");
        info.opensslPrivateKey = strdup("-----BEGIN PRIVATE KEY-----");
        info.clientCertificateString = strdup("-----BEGIN CERTIFICATE-----");
        info.authType = ADUC_AuthType_SASToken;
        info.connType = ADUC_ConnType_Device;

        ADUC_ConnectionInfo_DeAlloc(&info);

        CHECK(info.connectionString == nullptr);
        CHECK(info.certificateString == nullptr);
        CHECK(info.opensslEngine == nullptr);
        CHECK(info.opensslPrivateKey == nullptr);
        CHECK(info.clientCertificateString == nullptr);
        CHECK(info.authType == ADUC_AuthType_NotSet);
        CHECK(info.connType == ADUC_ConnType_NotSet);
    }

    SECTION("Handles already-NULL fields gracefully")
    {
        ADUC_ConnectionInfo info = {};
        info.connectionString = nullptr;
        info.certificateString = nullptr;
        info.opensslEngine = nullptr;
        info.opensslPrivateKey = nullptr;
        info.clientCertificateString = nullptr;
        info.authType = ADUC_AuthType_X509;
        info.connType = ADUC_ConnType_Module;

        // Should not crash when freeing NULL pointers.
        ADUC_ConnectionInfo_DeAlloc(&info);

        CHECK(info.connectionString == nullptr);
        CHECK(info.certificateString == nullptr);
        CHECK(info.opensslEngine == nullptr);
        CHECK(info.opensslPrivateKey == nullptr);
        CHECK(info.clientCertificateString == nullptr);
        CHECK(info.authType == ADUC_AuthType_NotSet);
        CHECK(info.connType == ADUC_ConnType_NotSet);
    }

    SECTION("Handles partial NULL fields")
    {
        ADUC_ConnectionInfo info = {};
        info.connectionString = strdup("HostName=partial.azure-devices.net");
        info.certificateString = nullptr;
        info.opensslEngine = strdup("engine");
        info.opensslPrivateKey = nullptr;
        info.clientCertificateString = nullptr;
        info.authType = ADUC_AuthType_SASCert;
        info.connType = ADUC_ConnType_Device;

        ADUC_ConnectionInfo_DeAlloc(&info);

        CHECK(info.connectionString == nullptr);
        CHECK(info.certificateString == nullptr);
        CHECK(info.opensslEngine == nullptr);
        CHECK(info.opensslPrivateKey == nullptr);
        CHECK(info.clientCertificateString == nullptr);
        CHECK(info.authType == ADUC_AuthType_NotSet);
        CHECK(info.connType == ADUC_ConnType_NotSet);
    }
}

// =====================================================================
// ADUC_IsValidUpdateId tests
// =====================================================================

TEST_CASE("ADUC_IsValidUpdateId", "[adu_types]")
{
    SECTION("Returns true for a fully populated UpdateId")
    {
        ADUC_UpdateId updateId = {};
        updateId.Provider = strdup("Microsoft");
        updateId.Name = strdup("TestUpdate");
        updateId.Version = strdup("1.0.0");

        CHECK(ADUC_IsValidUpdateId(&updateId) == true);

        free(updateId.Provider);
        free(updateId.Name);
        free(updateId.Version);
    }

    SECTION("Returns false when updateId is NULL")
    {
        CHECK(ADUC_IsValidUpdateId(nullptr) == false);
    }

    SECTION("Returns false when Provider is NULL")
    {
        ADUC_UpdateId updateId = {};
        updateId.Provider = nullptr;
        updateId.Name = strdup("TestUpdate");
        updateId.Version = strdup("1.0.0");

        CHECK(ADUC_IsValidUpdateId(&updateId) == false);

        free(updateId.Name);
        free(updateId.Version);
    }

    SECTION("Returns false when Provider is empty")
    {
        ADUC_UpdateId updateId = {};
        updateId.Provider = strdup("");
        updateId.Name = strdup("TestUpdate");
        updateId.Version = strdup("1.0.0");

        CHECK(ADUC_IsValidUpdateId(&updateId) == false);

        free(updateId.Provider);
        free(updateId.Name);
        free(updateId.Version);
    }

    SECTION("Returns false when Name is NULL")
    {
        ADUC_UpdateId updateId = {};
        updateId.Provider = strdup("Microsoft");
        updateId.Name = nullptr;
        updateId.Version = strdup("1.0.0");

        CHECK(ADUC_IsValidUpdateId(&updateId) == false);

        free(updateId.Provider);
        free(updateId.Version);
    }

    SECTION("Returns false when Name is empty")
    {
        ADUC_UpdateId updateId = {};
        updateId.Provider = strdup("Microsoft");
        updateId.Name = strdup("");
        updateId.Version = strdup("1.0.0");

        CHECK(ADUC_IsValidUpdateId(&updateId) == false);

        free(updateId.Provider);
        free(updateId.Name);
        free(updateId.Version);
    }

    SECTION("Returns false when Version is NULL")
    {
        ADUC_UpdateId updateId = {};
        updateId.Provider = strdup("Microsoft");
        updateId.Name = strdup("TestUpdate");
        updateId.Version = nullptr;

        CHECK(ADUC_IsValidUpdateId(&updateId) == false);

        free(updateId.Provider);
        free(updateId.Name);
    }

    SECTION("Returns false when Version is empty")
    {
        ADUC_UpdateId updateId = {};
        updateId.Provider = strdup("Microsoft");
        updateId.Name = strdup("TestUpdate");
        updateId.Version = strdup("");

        CHECK(ADUC_IsValidUpdateId(&updateId) == false);

        free(updateId.Provider);
        free(updateId.Name);
        free(updateId.Version);
    }

    SECTION("Returns false when all fields are NULL")
    {
        ADUC_UpdateId updateId = {};
        updateId.Provider = nullptr;
        updateId.Name = nullptr;
        updateId.Version = nullptr;

        CHECK(ADUC_IsValidUpdateId(&updateId) == false);
    }

    SECTION("Returns false when all fields are empty strings")
    {
        ADUC_UpdateId updateId = {};
        updateId.Provider = strdup("");
        updateId.Name = strdup("");
        updateId.Version = strdup("");

        CHECK(ADUC_IsValidUpdateId(&updateId) == false);

        free(updateId.Provider);
        free(updateId.Name);
        free(updateId.Version);
    }
}

// =====================================================================
// ADUC_UpdateId_UninitAndFree tests
// =====================================================================

TEST_CASE("ADUC_UpdateId_UninitAndFree", "[adu_types]")
{
    SECTION("Frees a fully populated UpdateId without crash")
    {
        // Use calloc to simulate how UpdateId is typically allocated.
        ADUC_UpdateId* updateId = static_cast<ADUC_UpdateId*>(calloc(1, sizeof(ADUC_UpdateId)));
        REQUIRE(updateId != nullptr);

        updateId->Provider = strdup("Microsoft");
        updateId->Name = strdup("TestUpdate");
        updateId->Version = strdup("1.0.0");

        // Should not crash; memory is freed.
        ADUC_UpdateId_UninitAndFree(updateId);
        // updateId is now freed, no further access.
    }

    SECTION("Handles NULL updateId gracefully")
    {
        // Should not crash when passed NULL.
        ADUC_UpdateId_UninitAndFree(nullptr);
    }

    SECTION("Handles UpdateId with NULL fields")
    {
        ADUC_UpdateId* updateId = static_cast<ADUC_UpdateId*>(calloc(1, sizeof(ADUC_UpdateId)));
        REQUIRE(updateId != nullptr);

        updateId->Provider = nullptr;
        updateId->Name = nullptr;
        updateId->Version = nullptr;

        // Should not crash when freeing NULL fields.
        ADUC_UpdateId_UninitAndFree(updateId);
    }

    SECTION("Handles UpdateId with partial NULL fields")
    {
        ADUC_UpdateId* updateId = static_cast<ADUC_UpdateId*>(calloc(1, sizeof(ADUC_UpdateId)));
        REQUIRE(updateId != nullptr);

        updateId->Provider = strdup("Microsoft");
        updateId->Name = nullptr;
        updateId->Version = strdup("2.0.0");

        ADUC_UpdateId_UninitAndFree(updateId);
    }
}

// =====================================================================
// ADUCITF_StateToString tests
// =====================================================================

TEST_CASE("ADUCITF_StateToString", "[adu_types]")
{
    SECTION("Returns correct string for ADUCITF_State_None")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_None), Equals("None"));
    }

    SECTION("Returns correct string for ADUCITF_State_Idle")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_Idle), Equals("Idle"));
    }

    SECTION("Returns correct string for ADUCITF_State_DownloadStarted")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_DownloadStarted), Equals("DownloadStarted"));
    }

    SECTION("Returns correct string for ADUCITF_State_DownloadSucceeded")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_DownloadSucceeded), Equals("DownloadSucceeded"));
    }

    SECTION("Returns correct string for ADUCITF_State_BackupStarted")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_BackupStarted), Equals("BackupStarted"));
    }

    SECTION("Returns correct string for ADUCITF_State_BackupSucceeded")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_BackupSucceeded), Equals("BackupSucceeded"));
    }

    SECTION("Returns correct string for ADUCITF_State_InstallStarted")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_InstallStarted), Equals("InstallStarted"));
    }

    SECTION("Returns correct string for ADUCITF_State_InstallSucceeded")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_InstallSucceeded), Equals("InstallSucceeded"));
    }

    SECTION("Returns correct string for ADUCITF_State_RestoreStarted")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_RestoreStarted), Equals("RestoreStarted"));
    }

    SECTION("Returns correct string for ADUCITF_State_ApplyStarted")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_ApplyStarted), Equals("ApplyStarted"));
    }

    SECTION("Returns correct string for ADUCITF_State_DeploymentInProgress")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_DeploymentInProgress), Equals("DeploymentInProgress"));
    }

    SECTION("Returns correct string for ADUCITF_State_Failed")
    {
        CHECK_THAT(ADUCITF_StateToString(ADUCITF_State_Failed), Equals("Failed"));
    }

    SECTION("Returns '<Unknown>' for an out-of-range value")
    {
        CHECK_THAT(ADUCITF_StateToString(static_cast<ADUCITF_State>(999)), Equals("<Unknown>"));
    }
}

// =====================================================================
// ADUCITF_UpdateActionToString tests
// =====================================================================

TEST_CASE("ADUCITF_UpdateActionToString", "[adu_types]")
{
    SECTION("Returns correct string for ADUCITF_UpdateAction_Invalid_Download")
    {
        CHECK_THAT(
            ADUCITF_UpdateActionToString(ADUCITF_UpdateAction_Invalid_Download), Equals("Invalid (Download)"));
    }

    SECTION("Returns correct string for ADUCITF_UpdateAction_Invalid_Install")
    {
        CHECK_THAT(
            ADUCITF_UpdateActionToString(ADUCITF_UpdateAction_Invalid_Install), Equals("Invalid (Install)"));
    }

    SECTION("Returns correct string for ADUCITF_UpdateAction_Invalid_Apply")
    {
        CHECK_THAT(ADUCITF_UpdateActionToString(ADUCITF_UpdateAction_Invalid_Apply), Equals("Invalid (Apply)"));
    }

    SECTION("Returns correct string for ADUCITF_UpdateAction_ProcessDeployment")
    {
        CHECK_THAT(
            ADUCITF_UpdateActionToString(ADUCITF_UpdateAction_ProcessDeployment), Equals("ProcessDeployment"));
    }

    SECTION("Returns correct string for ADUCITF_UpdateAction_Cancel")
    {
        CHECK_THAT(ADUCITF_UpdateActionToString(ADUCITF_UpdateAction_Cancel), Equals("Cancel"));
    }

    SECTION("Returns correct string for ADUCITF_UpdateAction_Undefined")
    {
        CHECK_THAT(ADUCITF_UpdateActionToString(ADUCITF_UpdateAction_Undefined), Equals("Undefined"));
    }

    SECTION("Returns '<Unknown>' for an out-of-range value")
    {
        CHECK_THAT(ADUCITF_UpdateActionToString(static_cast<ADUCITF_UpdateAction>(42)), Equals("<Unknown>"));
    }
}
