#include <aduc/result.h>

#include <do_config.h>
#include <do_download.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//fwd-decl
ADUC_Result DownloadRootKeyPkg_DO(const char* url, const char* targetFilePath);

//decls
typedef int32_t ADUC_Result_t;
typedef enum
{
    SimMode_NONE = 0,
    SimMode_DOWNLOAD_ROOTKEY_PKG,
    SimMode_DOWNLOAD_UPDATE_PAYLOAD,
} SimMode;

//globals
SimMode g_SimMode = SimMode_NONE;
char* g_arg_url = NULL;
char* g_arg_out_filepath = NULL;

bool parse_args(int argc, char** argv)
{
    bool parsed = false;

    if (argc != 4 && argc != 5)
    {
        return false;
    }

    // argv[0] (--rootkeypkg-download|--payload-download) [--simulate-bad-hash] <URL> <FILEPATH>
    int non_switch_cnt = 0;
    for (int i = 1; i < argc; ++i)
    {
        if (0 == strcmp(argv[i], "--rootkeypkg-download"))
        {
            g_SimMode = SimMode_DOWNLOAD_ROOTKEY_PKG;
        }
        else if (0 == strcmp(argv[i], "--rootkeypkg-download"))
        {
            g_SimMode = SimMode_DOWNLOAD_UPDATE_PAYLOAD;
        }
        else
        {
            size_t len = strlen(argv[i]);
            ++non_switch_cnt;
            if (non_switch_cnt > 2)
            {
                fprintf(stderr, "Too many non-switch args!\n");
                goto done;
            }
            switch (non_switch_cnt)
            {
            case 1:
                g_arg_url = (char*)calloc(1 + len, 1);
                if (NULL == g_arg_url)
                {
                    goto done;
                }
                strncpy(g_arg_url /*dest*/, argv[i] /*src*/, len);
                printf("Parsed URL arg: '%s'\n", g_arg_url);
                break;
            case 2:
                g_arg_out_filepath = (char*)calloc(1 + len, 1);
                if (NULL == g_arg_out_filepath)
                {
                    goto done;
                }
                strncpy(g_arg_out_filepath /*dest*/, argv[i] /*src*/, len);
                printf("Parsed FILE arg: '%s'\n", g_arg_out_filepath);
                break;
            default:
                goto done;
            }
        }
    }
    parsed = true;
done:
    return parsed;
}

void usage(const char* argv0)
{
    fprintf(
        stderr,
        "\nUsage: %s (--rootkeypkg-download|--payload-download) [--simulate-bad-hash] <URL> <FILEPATH>\n\n",
        argv0);
}

int main(int argc, char** argv)
{
    ADUC_Result result = {};

    if (!parse_args(argc, argv))
    {
        usage(argv[0]);
        exit(1);
    }

    if (g_SimMode == SimMode_DOWNLOAD_ROOTKEY_PKG)
    {
        result = DownloadRootKeyPkg_DO(g_arg_url, g_arg_out_filepath);
    }
    else if (g_SimMode == SimMode_DOWNLOAD_UPDATE_PAYLOAD)
    {
        // TODO
        fprintf(stderr, "\nDownload Update Payload NOT SUPPORTED YET\n");
    }

    printf(
        "ResultCode: %d(0x%08x)\nERC: %d(0x%08x)\n",
        result.ResultCode,
        result.ResultCode,
        result.ExtendedResultCode,
        result.ExtendedResultCode);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        return 1;
    }
    else
    {
        printf("Download Succeeded\n");
        return 0;
    }
}

// Ported from src/utils/rootkeypackage_utils/src/rootkeypackage_do_download.cpp
ADUC_Result DownloadRootKeyPkg_DO(const char* url, const char* targetFilePath)
{
    ADUC_Result result = {};

    printf("DownloadRootKeyPkg_DO - Downloading File '%s' to '%s'\n", url, targetFilePath);

    try
    {
        const std::chrono::seconds max_time_to_download = std::chrono::seconds(30); // normally, it is one hour
        const std::error_code ret =
            microsoft::deliveryoptimization::download::download_url_to_path(url, targetFilePath, max_time_to_download);
        if (ret.value() == 0)
        {
            result.ResultCode = ADUC_GeneralResult_Success;
        }
        else
        {
            fprintf(
                stderr,
                "DO error, msg: %s, code: %#08x, timeout? %d\n",
                ret.message().c_str(),
                ret.value(),
                ret == std::errc::timed_out);
            result.ExtendedResultCode = MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_ADUC_FACILITY_INFRA_MGMT(
                ADUC_COMPONENT_ROOTKEY_DOWNLOADER, ret.value());
        }
    }
    catch (...)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_DOWNLOAD_EXCEPTION;
    }

    printf("Download rc: %d, erc: 0x%08x\n", result.ResultCode, result.ExtendedResultCode);

    return result;
}
