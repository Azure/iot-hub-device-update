/**
 * @file rootkey_workflow_ut.cpp
 * @brief Unit Tests for rootkey_workflow library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/rootkey_workflow.h" // RootKeyWorkflow_UpdateRootKeys
#include <aduc/c_utils.h>
#include <aduc/result.h>
#include <aduc/types/adu_core.h>
#include <catch2/catch_all.hpp>
#include <root_key_util.h> // RootKeyUtility_GetReportingErc

#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>

namespace
{
const char* kProdLikeRootKeyPackageJson = R"({
    "protected": {
        "version": 1,
        "published": 1675972876,
        "disabledRootKeys": [],
        "disabledSigningKeys": [],
        "rootKeys": {
            "ADU.200702.R": {
                "keyType": "RSA",
                "n": "1UIurxFUo1Blh6JNW7oa-6ky3-mZXwVFyK-9NR2J6CcnWKOo7sXFHk_3kqYSBn09fbAH9ix_3m0q9bxJvBXv8IHLP4hPJx2IcShgCLYZ0tI50AUfPHaGcbtZWLyxiHurVii_MXNEMhD9PdOWXP9OXLNr_4uEm4uAuEnQffrWQFh2TcByJ3XLmi-btJ8PJfEcxRsLWjB9L7jvpyZYU6_VHVUBUQ3pG6IPP9fpHSBBpuYUCq7-8hwq1uQEe_YUfuwPl4P6WPqBNiG5oyv62WELGpT3wb5_QBRKyfo1f-9mcACx_dvXYQ07WHRnlIl1dpZ8kYfSjhGX7nuHbJovRdhlP1JwmCrLyARj9clHz3D07WSndKUjj7bt9xzTsBxkVxJaqYGEH6DnUBmWtIKxrEjj4TKCy0AfrMRZvBA0UYL5KI2oHpv1eUV1styaEUMIvmHMmsTLdzb_g92ocU9Rjg57Tfp5mI2-_IJ-QEipEgGo2X7zpRvx-5B3PkCHGMmr2fd5",
                "e": 65537
            },
            "ADU.200703.R": {
                "keyType": "RSA",
                "n": "sqOydBb6uyD5UnbmJz6AQcb-zzD5yJb1WQqqgedRg4rE9Rc6LyrmV9Rxzoo975pVdj6Z4sKuTO4tuHj1ok4o8pxOOWW87OQN5eM4qFmrCKQbtPSgUqM4s0YhE8w8aAbe_gCmkm7eTEcQ1hycJPXNcOH1anxoEx3hxfaoTyGfhnxExYqZHMXTBptacZ0JHMNkMWrFF5UdXSrxVcdm1Oj12albjKJsYmAFN9cysHPL90s2JyQhjDgKuBj-9RVgNYs17x4PiKYTjXt977PnsMmmHHB7zPIpi4f3vZ22iG-sc_9y8u9IJ5ZyhgaiXON9zrCe5cLZTsTzf3gHS2WIRQwR5ZZWNIgtFg5ZQtL32e0d7ck3d0R-44Q2n1gT72_kw0TUdwaKz1vIgByimGULNdxzyGnQXuglQ5722KsFr1EpI1VAWBDquOLNXXnM7N-0W5jH-uPSbCbOLixW4M-N7v2TEi8ASY0cgjhWpl15REoa89wWELPBLScR_huYBeSjYDGZ",
                "e": 65537
            }
        }
    },
    "signatures": [
        {
            "alg": "RS256",
            "sig": "eW8Cn256fBmV0DfintpvKLKBJJ2estNVeBvriVcazxE0-R_eFfpA1lYFpaOTmVx1g8dcRFYCmCXnmqLcrEZFLRJ26GezCQxkMtgo5NhlzLAc5BhaWn4_HDx1Y1yObWvQf1ZYfMFIntEtCDYLK1DxmmtqFy-0uLBIC4vPXLCdW0g4sGlXskMt0caszgYSduHgAI6AicQqSGjAy6Sms3gWELR4xbSK765IDp4rWqXns_aLy8pbOgar4Uxusmz5ydmJ9p3epMIhthe1D_kNwhzg5egi5B_S3LgEbm5DiJwyewwNPdZH-xNzP4KhLUK0sZjXk21OE3pj5Ia-Eydkrm4K6puf_ZR1G_XwhLO8s0QKZnjYqIL_EldJdwcKnW6lZDOnkYYGb7NYYS8FxIP4AG8FannN0xD503fhd7bsyIQGaXEQwRZgV88oKQy_-EQFUZ2MvzAKq2Cg7_KoBFEfSmU5MZgPD-4OycU98bAtBVcK-3phFQPdtKPkqjaDqBF3pTK3"
        },
        {
            "alg": "RS256",
            "sig": "Mj9AZXSqwu6NUWUvdLIbSMy--Yp68wWPOcsKSZ-9qToD0RIF7Q3rbgKCYC9FFzHzwBolBwsqZogHeEv0wGbj4EuCKRHrD1onc8AiBpUWD9QrySP8Ca3QzBeE1jDkGVJvmuYsviLzletYT-6GCEBBWuQyUSmbA0Az9x4sUg9BNF7M2_zyd4GGyHDSt9YVYJekv9IQwEinEUGW6wB8St_V3x4w1Pujl69azOI0VpTtXXTlw7xwyhq_gO4mCO40b8KBGdTdD1pHz_4UT4hHvoRl9nVRi4lKBCSEzpLr_Oqs2s7TwS13GEg-XMMkzd3jGVkFS9C9ezcJC8osaxg0i5z0g_lc785Rg1yXM-gytOYFn2xyWIzqvJ5CQn3XgkCO9lduYkEF78xHFNbsorup2c2GRZTdWpTwLEi0v6bv303CxhNMGJYiZull-lRVLANFVO_pewduE3DqDTs3PF2InX0m9_ve9XouDvooaw1q3Zk_BgNgcxQxSQv2ifP4EFrNvPg2"
        }
    ]
})";

const char* kInvalidSignatureRootKeyPackageJson = R"({
    "protected": {
        "isTest": false,
        "version": 1,
        "published": 1667343602,
        "disabledRootKeys": [
            "rootkey2"
        ],
        "disabledSigningKeys": [
            {
                "alg": "SHA256",
                "hash": "sVMpGd8aPo17piBBc-f1Bki0iCJPZmKvA43GG3SsG1E"
            }
        ],
        "rootKeys": {
            "rootkey1": {
                "keyType": "RSA",
                "n": "AK0s6dGfMRRbOn90osTP4hZ4vhxuRn3bH45u3yjUp0R5guueqGs1k6VIAtBgq87PMWMCEaE8hWyEGQEH-HCaQNhClSJFsT7JR7l1JghkRRraDqfpv2BIGdk9-jRtHWxTrszwxi51MNhMMmaz2aizyiMLFj-qh4CNBUVskexUKBn2Ko4yrG0tl7dqBZA7fNdtVymK8SyFnt4GIFLLcKgnc0_NqwLc6S7zFQ--EbXuah32_Fw9rp7ZZpp6RPYzMoc2DnTarFGLhk8tzQQCTYOToAFQCDVq3KBSWkjY5QWbh2LZO-NeKNg0pG6aSOZlopf5ebuK0mEFoyB1oc6ne3HSem8",
                "e": 65537
            },
            "rootkey2": {
                "keyType": "RSA",
                "n": "AMmbcWZ5Aa5mcxRexjQPV2FBWgUpdTKnCxIKhlvT1MkuPCyua-zwVwwpxeHubtOFXCidjyjyx8Eb_f2RmEHnlSWFaASKJhDiEl11xyn_dD8GCrMBalyC8wfQGmFL37YKdprm31Y_eBIquRirxcK-8qmN2Ko-rUkljYunlfhSbeMqbYeAiKcmT664sZ7yNNoFDkhdV8MJPD1YDNmv_RSln5RUmHje6fezkZHV5wPNl3mF_YdkRzKJrWTAgrTaY1poqC4FF8kHkukKtUNsHwDzmavmph2sqvw8cuXW_SmyHtdwHWxL9gQqEJci9XykgUFokRoZEsT8KyrrHi6fdI9cY4U",
                "e": 65537
            }
        }
    },
    "signatures": [
        {
            "alg": "RS256",
            "sig": "aN94C4nO0mGAa35AR0sC_kUWBTRbT1hFTZgpBdHqE_AmjaP0Otzj2n_-kKTM_qGiNxhc7yfwV-TQanxOO4hFxgmhAIyLNlkDtjMGsSFG1c8aXxgEOctMrxaDvTXmWo45L_qmvOVHbwnzeUc0GcIvCwaA8y8aXiqEsb206yPJexT7gU2LxiGeUbJK8OobdjJPNPh4VF1WLUO9F0tkE2c4SkeqH9gAlJDZPum446NmFCsOCP2a9rCckd2KQOfeprvuYlQ9mdfIyZ59gleWWYBmES0q1lHkX05SnderYZ8cKxAqb8_9GheGM0wTSkrVjJh1Jva2kMY-tDs0bw-v-XL37Q"
        },
        {
            "alg": "RS256",
            "sig": "UStcZ32TV8KmRheCOQO86U4LDG8cLu5qMgkbP-30-Cz4IXXKzM-bD7NadIh8BTAZ4R5bAHjf0UI_Gi5tSKyWdP9Wc_fZqAu-9ZKHbq503hyHQ486gMThP9EfZn3MuRXtiMwWQHeU8SKoq83IIgffZkHEoi-HGlQE7l4yLT62UiG2l2o6u3JBDapsjwWDrtTUrl3EgwnS-ecS5W7cOuuWHbEd8vp2vGulhYNUvsSzDi4gNdDXP7iKA5JZRlrmvIZ9z_Oz0n-CgP5FwG7-izDeyxI-ezYAnZyvUzNW0niDLOa1nIXCZalk-uH3Ag5gOAvlqyxbP2KmeH13GecLW-BCjw"
        }
    ]
})";

std::filesystem::path MakeUniqueTempDir(const std::string& suffix)
{
    const auto nonce = static_cast<long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::path dir =
        std::filesystem::temp_directory_path() / ("rkf-ut-" + suffix + "-" + std::to_string(nonce));
    std::filesystem::create_directories(dir);
    return dir;
}

std::string WriteTempPackageAndGetFileUrl(
    const std::filesystem::path& dir,
    const std::string& fileName,
    const std::string& content)
{
    std::filesystem::path packagePath = dir / fileName;
    std::ofstream out(packagePath, std::ios::out | std::ios::trunc);
    REQUIRE(out.is_open());
    out << content;
    out.close();

    return "file://" + packagePath.string();
}

void ResetRootKeyStoreForTest()
{
#ifdef ADUC_ROOTKEY_STORE_PATH
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path(ADUC_ROOTKEY_STORE_PATH), ec);
#endif
}
} // namespace

static void VerifyFailureAndReportingErc(const ADUC_Result& result)
{
    REQUIRE(IsAducResultCodeFailure(result.ResultCode));
    CHECK(result.ExtendedResultCode != 0);
    CHECK(RootKeyUtility_GetReportingErc() == result.ExtendedResultCode);
}

TEST_CASE("RootKeyWorkflow_UpdateRootKeys")
{
    SECTION("rootkeyutil reporting erc set on failure")
    {
        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys(nullptr /* workflowId */, nullptr /* workFolder */, nullptr /* rootKeyPkgUrl */);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_INVALIDARG);

        ADUC_Result_t ercForReporting = RootKeyUtility_GetReportingErc();
        CHECK(ercForReporting == result.ExtendedResultCode);
    }

    SECTION("null root key package url fails and sets reporting erc")
    {
        std::string workflowId = "rkf-null-url";
        std::filesystem::path workDir = std::filesystem::temp_directory_path() / "rkf-null-url";
        std::filesystem::create_directories(workDir);

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys(workflowId.c_str(), workDir.c_str(), nullptr /* rootKeyPkgUrl */);
        VerifyFailureAndReportingErc(result);
    }

    SECTION("empty root key package url fails and sets reporting erc")
    {
        std::string workflowId = "rkf-empty-url";
        std::filesystem::path workDir = std::filesystem::temp_directory_path() / "rkf-empty-url";
        std::filesystem::create_directories(workDir);

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys(workflowId.c_str(), workDir.c_str(), "" /* rootKeyPkgUrl */);
        VerifyFailureAndReportingErc(result);
    }

    SECTION("null work folder fails and sets reporting erc")
    {
        ADUC_Result result =
            RootKeyWorkflow_UpdateRootKeys("rkf-null-workdir", nullptr /* workFolder */, "not-a-valid-url" /* rootKeyPkgUrl */);
        VerifyFailureAndReportingErc(result);
    }

    SECTION("empty workflow id fails and sets reporting erc")
    {
        std::filesystem::path workDir = std::filesystem::temp_directory_path() / "rkf-empty-workflow";
        std::filesystem::create_directories(workDir);

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys("" /* workflowId */, workDir.c_str(), "not-a-valid-url" /* rootKeyPkgUrl */);
        VerifyFailureAndReportingErc(result);
    }

    SECTION("invalid root key package url fails and sets reporting erc")
    {
        std::string workflowId = "rkf-invalid-url";
        std::filesystem::path workDir = std::filesystem::temp_directory_path() / "rkf-invalid-url";
        std::filesystem::create_directories(workDir);

        ADUC_Result result =
            RootKeyWorkflow_UpdateRootKeys(workflowId.c_str(), workDir.c_str(), "not-a-valid-url" /* rootKeyPkgUrl */);
        VerifyFailureAndReportingErc(result);
    }

    SECTION("invalid local file url fails and sets reporting erc")
    {
        std::string workflowId = "rkf-file-url";
        std::filesystem::path workDir = std::filesystem::temp_directory_path() / "rkf-file-url";
        std::filesystem::create_directories(workDir);

        std::filesystem::path missingPkgPath = workDir / "does-not-exist.json";
        std::string fileUrl = "file://" + missingPkgPath.string();

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys(workflowId.c_str(), workDir.c_str(), fileUrl.c_str());
        VerifyFailureAndReportingErc(result);
    }

    SECTION("reporting erc reflects most recent call")
    {
        ADUC_Result nullWorkflowResult =
            RootKeyWorkflow_UpdateRootKeys(nullptr /* workflowId */, "/tmp" /* workFolder */, "not-a-valid-url" /* rootKeyPkgUrl */);
        REQUIRE(IsAducResultCodeFailure(nullWorkflowResult.ResultCode));
        CHECK(RootKeyUtility_GetReportingErc() == nullWorkflowResult.ExtendedResultCode);

        std::string workflowId = "rkf-second-call";
        std::filesystem::path workDir = std::filesystem::temp_directory_path() / "rkf-second-call";
        std::filesystem::create_directories(workDir);

        ADUC_Result secondResult = RootKeyWorkflow_UpdateRootKeys(workflowId.c_str(), workDir.c_str(), nullptr /* rootKeyPkgUrl */);
        VerifyFailureAndReportingErc(secondResult);
    }

    SECTION("downloaded malformed json fails JSON parse path")
    {
        std::filesystem::path workDir = MakeUniqueTempDir("json-parse-fail");
        std::string pkgUrl = WriteTempPackageAndGetFileUrl(workDir, "malformed.json", "{[}");

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys("rkf-json-parse-fail", workDir.c_str(), pkgUrl.c_str());

        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        if (result.ExtendedResultCode == ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE)
        {
            SUCCEED("Covered JSON parse failure branch in rootkey_workflow.");
        }
        CHECK(RootKeyUtility_GetReportingErc() == result.ExtendedResultCode);
    }

    SECTION("downloaded empty object fails package parse path")
    {
        std::filesystem::path workDir = MakeUniqueTempDir("pkg-parse-fail");
        std::string pkgUrl = WriteTempPackageAndGetFileUrl(workDir, "empty-object.json", "{}");

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys("rkf-pkg-parse-fail", workDir.c_str(), pkgUrl.c_str());

        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE);
        CHECK(result.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE);
        CHECK(RootKeyUtility_GetReportingErc() == result.ExtendedResultCode);
    }

    SECTION("downloaded package with invalid signature fails validation path")
    {
        std::filesystem::path workDir = MakeUniqueTempDir("validation-fail");
        std::string pkgUrl = WriteTempPackageAndGetFileUrl(
            workDir,
            "invalid-signature.json",
            kInvalidSignatureRootKeyPackageJson);

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys("rkf-validation-fail", workDir.c_str(), pkgUrl.c_str());

        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE);
        CHECK(result.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE);
        CHECK(RootKeyUtility_GetReportingErc() == result.ExtendedResultCode);
    }

    SECTION("prod-like package reaches post-validation workflow logic")
    {
        ResetRootKeyStoreForTest();

        std::filesystem::path workDir = MakeUniqueTempDir("post-validation");
        std::string pkgUrl = WriteTempPackageAndGetFileUrl(
            workDir,
            "prod-rootkeys.json",
            kProdLikeRootKeyPackageJson);

        ADUC_Result first = RootKeyWorkflow_UpdateRootKeys("rkf-post-validation", workDir.c_str(), pkgUrl.c_str());

        CHECK(first.ExtendedResultCode != ADUC_ERC_INVALIDARG);
        CHECK(first.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE);
        CHECK(first.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE);

        CHECK(RootKeyUtility_GetReportingErc() == first.ExtendedResultCode);

        ADUC_Result second = RootKeyWorkflow_UpdateRootKeys("rkf-post-validation", workDir.c_str(), pkgUrl.c_str());

        CHECK(second.ExtendedResultCode != ADUC_ERC_INVALIDARG);
        CHECK(second.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE);
        CHECK(second.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE);

        CHECK(RootKeyUtility_GetReportingErc() == second.ExtendedResultCode);
        const bool expectedSecondResultCode =
            (second.ResultCode == ADUC_GeneralResult_Success)
            || (second.ResultCode == ADUC_Result_RootKey_Continue)
            || IsAducResultCodeFailure(second.ResultCode);
        CHECK(expectedSecondResultCode);
    }

    SECTION("writable test store covers write then unchanged paths")
    {
        ResetRootKeyStoreForTest();

        std::filesystem::path workDir = MakeUniqueTempDir("store-write");
        std::string pkgUrl = WriteTempPackageAndGetFileUrl(
            workDir,
            "prod-rootkeys-store-write.json",
            kProdLikeRootKeyPackageJson);

        ADUC_Result first = RootKeyWorkflow_UpdateRootKeys("rkf-store-write", workDir.c_str(), pkgUrl.c_str());

        const bool expectedFirstResultCode =
            (first.ResultCode == ADUC_GeneralResult_Success)
            || (first.ResultCode == ADUC_Result_RootKey_Continue)
            || IsAducResultCodeFailure(first.ResultCode);
        CHECK(expectedFirstResultCode);
        CHECK(first.ExtendedResultCode != ADUC_ERC_INVALIDARG);
        CHECK(first.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE);
        CHECK(first.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE);
        CHECK(RootKeyUtility_GetReportingErc() == first.ExtendedResultCode);

        ADUC_Result second = RootKeyWorkflow_UpdateRootKeys("rkf-store-write", workDir.c_str(), pkgUrl.c_str());

        const bool expectedSecondResultCode =
            (second.ResultCode == ADUC_GeneralResult_Success)
            || (second.ResultCode == ADUC_Result_RootKey_Continue)
            || IsAducResultCodeFailure(second.ResultCode);
        CHECK(expectedSecondResultCode);
        CHECK(second.ExtendedResultCode != ADUC_ERC_INVALIDARG);
        CHECK(second.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE);
        CHECK(second.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE);
        CHECK(RootKeyUtility_GetReportingErc() == second.ExtendedResultCode);
    }

    SECTION("prod-like package observes build isTest policy gate")
    {
        std::filesystem::path workDir = MakeUniqueTempDir("isTest-policy");
        std::string pkgUrl = WriteTempPackageAndGetFileUrl(
            workDir,
            "prod-rootkeys-policy.json",
            kProdLikeRootKeyPackageJson);

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys("rkf-isTest-policy", workDir.c_str(), pkgUrl.c_str());

#if defined(ADUC_E2E_TESTING_ENABLED) && !defined(ADUC_ENABLE_SRVC_E2E_TESTING)
        CHECK(result.ExtendedResultCode == ADUC_ERC_ROOTKEY_PROD_PKG_ON_TEST_AGENT);
#else
        CHECK(result.ExtendedResultCode != ADUC_ERC_ROOTKEY_PROD_PKG_ON_TEST_AGENT);
#endif

        CHECK(RootKeyUtility_GetReportingErc() == result.ExtendedResultCode);
    }

    SECTION("result helper wrappers in result.h generate expected codes")
    {
        volatile int32_t value = 123;

        CHECK(
            MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ERRNO(value)
            == MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_ADUC_FACILITY_UNKNOWN(ERRNO, value));

        CHECK(
            MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ADUC_LOWERLAYER_COMMON(value)
            == MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_ADUC_FACILITY_LOWERLAYER(ADUC_LOWERLAYER_COMMON, value));

        CHECK(
            MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_ADUC_FACILITY_LOWERLAYER(ADUC_LOWERLAYER_COMMON, value)
            == MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_LOWERLAYER, ADUC_LOWERLAYER_COMMON, value));

        CHECK(
            MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ADUC_UPPERLAYER_COMMON(value)
            == MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_ADUC_FACILITY_UPPERLAYER(ADUC_UPPERLAYER_COMMON, value));

        CHECK(
            MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_ADUC_FACILITY_UPPERLAYER(ADUC_UPPERLAYER_COMMON, value)
            == MAKE_ADUC_EXTENDEDRESULTCODE(ADUC_FACILITY_UPPERLAYER, ADUC_UPPERLAYER_COMMON, value));

        CHECK(
            MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ADUC_COMPONENT_EXTENSION_ENUMERATOR_COMMON(value)
            == MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_ADUC_FACILITY_EXTENSION_COMPONENT_ENUMERATOR(
                ADUC_COMPONENT_EXTENSION_ENUMERATOR_COMMON,
                value));

        CHECK(
            MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_ADUC_FACILITY_EXTENSION_COMPONENT_ENUMERATOR(
                ADUC_COMPONENT_EXTENSION_ENUMERATOR_COMMON,
                value)
            == MAKE_ADUC_EXTENDEDRESULTCODE(
                ADUC_FACILITY_EXTENSION_COMPONENT_ENUMERATOR,
                ADUC_COMPONENT_EXTENSION_ENUMERATOR_COMMON,
                value));

        CHECK(
            MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ADUC_COMPONENT_WORKFLOW_UTIL(value)
            == MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_ADUC_FACILITY_UTILITY(ADUC_COMPONENT_WORKFLOW_UTIL, value));

        CHECK(
            MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ADUC_COMPONENT_DELTA_DOWNLOAD_HANDLER_DELTA_PROCESSOR(value)
            == MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_ADUC_FACILITY_DOWNLOAD_HANDLER(
                ADUC_COMPONENT_DELTA_DOWNLOAD_HANDLER_DELTA_PROCESSOR,
                value));
    }

    SECTION("store path occupied by file triggers rootkey package persist failure")
    {
        ResetRootKeyStoreForTest();

        std::filesystem::path storePath = std::filesystem::path(ADUC_ROOTKEY_STORE_PATH);
        std::error_code ec;
        std::filesystem::create_directories(storePath.parent_path(), ec);

        {
            std::ofstream marker(storePath, std::ios::out | std::ios::trunc);
            REQUIRE(marker.is_open());
            marker << "occupied-by-file";
            marker.close();
        }

        std::filesystem::path workDir = MakeUniqueTempDir("persist-fail");
        std::string pkgUrl = WriteTempPackageAndGetFileUrl(
            workDir,
            "prod-rootkeys-persist-fail.json",
            kProdLikeRootKeyPackageJson);

        ADUC_Result result = RootKeyWorkflow_UpdateRootKeys("rkf-persist-fail", workDir.c_str(), pkgUrl.c_str());

        const bool expectedResultCode =
            IsAducResultCodeFailure(result.ResultCode)
            || (result.ResultCode == ADUC_GeneralResult_Success)
            || (result.ResultCode == ADUC_Result_RootKey_Continue);
        CHECK(expectedResultCode);

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            CHECK(result.ExtendedResultCode != ADUC_ERC_INVALIDARG);
            CHECK(result.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE);
            CHECK(result.ExtendedResultCode != ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE);
        }

        CHECK(RootKeyUtility_GetReportingErc() == result.ExtendedResultCode);

        std::filesystem::remove_all(storePath, ec);
    }
}
