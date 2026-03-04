/**
 * @file main.cpp
 * @brief Standalone tool that downloads a root key package from a URL
 * and runs it through the full rootkey validation flow.
 *
 * Usage:
 *   rootkey_validator [--url <URL>] [--workdir <DIR>] [--downloader curl|do]
 *
 * Defaults:
 *   URL:        http://granite-iothub-aat-dui--granite-iothub-aat-du.b.nlu.dl.adu.microsoft.com/SouthCentralUS/rootkeypackages/rootkeypackage-2.json
 *   workdir:    /tmp/rootkey_validator
 *   downloader: curl
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/c_utils.h>
#include <aduc/rootkeypackage_curl_download.h>
#include <aduc/rootkeypackage_download.h>

#if ADUC_HAVE_DO_DOWNLOAD
#include <aduc/rootkeypackage_do_download.h>
#endif
#include <aduc/rootkeypackage_parse.h>
#include <aduc/rootkeypackage_utils.h>
#include <azure_c_shared_utility/strings.h>
#include <parson.h>
#include <root_key_util.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#define DEFAULT_URL                                                                                                    \
    "http://granite-iothub-aat-dui--granite-iothub-aat-du.b.nlu.dl.adu.microsoft.com/SouthCentralUS/"                  \
    "rootkeypackages/rootkeypackage-2.json"

#define DEFAULT_WORKDIR "/tmp/rootkey_validator"
#define WORKFLOW_ID "rootkey-validator-tool"

enum DownloaderType
{
    Downloader_Curl,
#if ADUC_HAVE_DO_DOWNLOAD
    Downloader_DO,
#endif
};

static void print_usage(const char* argv0)
{
    fprintf(stderr, "\nUsage: %s [--url <URL>] [--workdir <DIR>] [--downloader curl|do]\n\n", argv0);
    fprintf(stderr, "  --url         URL of the root key package to download and validate.\n");
    fprintf(stderr, "                Default: %s\n\n", DEFAULT_URL);
    fprintf(stderr, "  --workdir     Working directory for downloads.\n");
    fprintf(stderr, "                Default: %s\n\n", DEFAULT_WORKDIR);
#if ADUC_HAVE_DO_DOWNLOAD
    fprintf(stderr, "  --downloader  Download method: 'curl' or 'do' (DeliveryOptimization).\n");
#else
    fprintf(stderr, "  --downloader  Download method: 'curl'.\n");
#endif
    fprintf(stderr, "                Default: curl\n\n");
}

static std::string slurp_file(const std::string& filepath)
{
    std::ifstream ifs(filepath);
    if (!ifs.is_open())
    {
        return {};
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

int main(int argc, char** argv)
{
    const char* url = DEFAULT_URL;
    const char* workdir = DEFAULT_WORKDIR;
    DownloaderType downloaderType = Downloader_Curl;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--url") == 0 && i + 1 < argc)
        {
            url = argv[++i];
        }
        else if (strcmp(argv[i], "--workdir") == 0 && i + 1 < argc)
        {
            workdir = argv[++i];
        }
        else if (strcmp(argv[i], "--downloader") == 0 && i + 1 < argc)
        {
            ++i;
            if (strcmp(argv[i], "curl") == 0)
            {
                downloaderType = Downloader_Curl;
            }
#if ADUC_HAVE_DO_DOWNLOAD
            else if (strcmp(argv[i], "do") == 0)
            {
                downloaderType = Downloader_DO;
            }
#endif
            else
            {
#if ADUC_HAVE_DO_DOWNLOAD
                fprintf(stderr, "Unknown downloader: '%s'. Must be 'curl' or 'do'.\n", argv[i]);
#else
                fprintf(stderr, "Unknown downloader: '%s'. Only 'curl' is supported in this build.\n", argv[i]);
#endif
                print_usage(argv[0]);
                return 1;
            }
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else
        {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

#if ADUC_HAVE_DO_DOWNLOAD
    const char* downloaderName = (downloaderType == Downloader_Curl) ? "Curl" : "DeliveryOptimization";
    RootKeyPkgDownloadFunc downloadFn =
        (downloaderType == Downloader_Curl) ? DownloadRootKeyPkg_Curl : DownloadRootKeyPkg_DO;
#else
    const char* downloaderName = "Curl";
    RootKeyPkgDownloadFunc downloadFn = DownloadRootKeyPkg_Curl;
#endif

    printf("=== Root Key Package Validator Tool ===\n\n");
    printf("URL:        %s\n", url);
    printf("WorkDir:    %s\n", workdir);
    printf("Downloader: %s\n\n", downloaderName);

    ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };
    STRING_HANDLE downloadedFile = nullptr;
    ADUC_RootKeyPackage rootKeyPackage{};
    bool rootKeyPackageInited = false;

    // ---- Step 1: Download ----
    printf("[Step 1/4] Downloading root key package using %s...\n", downloaderName);

    ADUC_RootKeyPkgDownloaderInfo downloaderInfo{
        downloaderName,
        downloadFn,
        workdir,
    };

    result = ADUC_RootKeyPackageUtils_DownloadPackage(url, WORKFLOW_ID, &downloaderInfo, &downloadedFile);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        fprintf(
            stderr,
            "[Step 1/4] FAILED: Download failed. ResultCode: %d, ERC: 0x%08x\n",
            result.ResultCode,
            result.ExtendedResultCode);
        goto done;
    }

    printf("[Step 1/4] SUCCESS: Downloaded to %s\n\n", STRING_c_str(downloadedFile));

    // ---- Step 2: Read JSON ----
    {
        printf("[Step 2/4] Reading and parsing JSON...\n");

        std::string jsonString = slurp_file(STRING_c_str(downloadedFile));
        if (jsonString.empty())
        {
            fprintf(stderr, "[Step 2/4] FAILED: Could not read file '%s'\n", STRING_c_str(downloadedFile));
            result.ResultCode = ADUC_GeneralResult_Failure;
            result.ExtendedResultCode = 0;
            goto done;
        }

        printf("[Step 2/4] SUCCESS: Read %zu bytes of JSON\n\n", jsonString.length());

        // ---- Step 3: Parse root key package ----
        printf("[Step 3/4] Parsing root key package structure...\n");

        result = ADUC_RootKeyPackageUtils_Parse(jsonString.c_str(), &rootKeyPackage);

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            fprintf(
                stderr,
                "[Step 3/4] FAILED: Parse failed. ResultCode: %d, ERC: 0x%08x\n",
                result.ResultCode,
                result.ExtendedResultCode);
            goto done;
        }

        rootKeyPackageInited = true;
        printf("[Step 3/4] SUCCESS: Root key package parsed successfully\n\n");
    }

    // ---- Step 4: Validate with hardcoded keys ----
    printf("[Step 4/4] Validating root key package with hardcoded keys...\n");

    result = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(&rootKeyPackage);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        fprintf(
            stderr,
            "[Step 4/4] FAILED: Validation failed. ResultCode: %d, ERC: 0x%08x\n",
            result.ResultCode,
            result.ExtendedResultCode);
        goto done;
    }

    printf("[Step 4/4] SUCCESS: Root key package is VALID\n\n");
    result.ResultCode = ADUC_GeneralResult_Success;

done:
    printf("=== Final Result ===\n");
    printf("ResultCode: %d (0x%08x)\n", result.ResultCode, result.ResultCode);
    printf("ERC:        %d (0x%08x)\n", result.ExtendedResultCode, result.ExtendedResultCode);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        printf("Overall:    FAILED\n");
    }
    else
    {
        printf("Overall:    PASSED\n");
    }

    STRING_delete(downloadedFile);

    if (rootKeyPackageInited)
    {
        ADUC_RootKeyPackageUtils_Destroy(&rootKeyPackage);
    }

    return IsAducResultCodeFailure(result.ResultCode) ? 1 : 0;
}