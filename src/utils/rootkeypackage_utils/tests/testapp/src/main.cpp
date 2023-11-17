/**
 * @file main.cpp
 * @brief The main entrypoint for the rootkeypackage test app.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/c_utils.h>
#include <aduc/file_test_utils.hpp>
#include <aduc/rootkeypackage_do_download.h>
#include <aduc/rootkeypackage_utils.h>
#include <azure_c_shared_utility/strings.h>
#include <string>

#define ROOTKEY_PKG_URL "http://localhost:8083/rootkey.json"
#define WORKFLOW_ID "7cf7241f-9ede-3e37-ca72-a7593bd7fc0f"

int main(int argc, char** argv)
{
    UNREFERENCED_PARAMETER(argc); // kept for posterity and it's a main() function
    UNREFERENCED_PARAMETER(argv);

    ADUC_Result result = { 0, 0 };
    int ret = 1;
    STRING_HANDLE downloadedFile = nullptr;

    std::string jsonString;
    std::string filepathStr;
    ADUC_RootKeyPackage rootKeyPackage{};

    ADUC_RootKeyPkgDownloaderInfo downloaderInfo{
        "DO",
        DownloadRootKeyPkg_DO,
        "/tmp/deviceupdate/rootkey_download_test_app",
    };

    result = ADUC_RootKeyPackageUtils_DownloadPackage(ROOTKEY_PKG_URL, WORKFLOW_ID, &downloaderInfo, &downloadedFile);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        printf("Download failed with erc 0x%08x.\n", result.ExtendedResultCode);
        ret = 1;
        goto done;
    }

    printf("Downloaded file to %s\n", STRING_c_str(downloadedFile));

    filepathStr = STRING_c_str(downloadedFile);
    jsonString = aduc::FileTestUtils_slurpFile(filepathStr);

    printf("Parsing root key package at %s ...\n", STRING_c_str(downloadedFile));

    result = ADUC_RootKeyPackageUtils_Parse(jsonString.c_str(), &rootKeyPackage);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        printf("Failed parse of root key package, erc 0x%08x.\n", result.ExtendedResultCode);
        ret = 1;
        goto done;
    }

    printf("Success!\n");
    ret = 0;

done:
    STRING_delete(downloadedFile);

    return ret;
}
