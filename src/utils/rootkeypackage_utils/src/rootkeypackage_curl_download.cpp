/**
 * @file rootkeypackage_curl_download.cpp
 * @brief Implements curl download of root key packages.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/types/adu_core.h>
#include <aduc/logging.h>
#include "aduc/process_utils.hpp" // for ADUC_LaunchChildProcess
#include <aduc/result.h>

#include <string>
#include <vector>

EXTERN_C_BEGIN

ADUC_Result DownloadRootKeyPkg_Curl(const char* url, const char* targetFilePath)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };

    Log_Info("Downloading File '%s' to '%s'", url, targetFilePath);

    try
    {
        std::vector<std::string> args;

        // -L (or --location). Handle 3xx redirects. See https://curl.se/docs/manpage.html#-L
        args.emplace_back("-L");

        // -C (or --continue-at). With hyphen after, e.g. '-C -', enables auto-resuming transfers. See https://curl.se/docs/manpage.html#-C
        args.emplace_back("-C");
        args.emplace_back("-");

        // -o (or --output). Output to file instead of stdout. See https://curl.se/docs/manpage.html#-o
        // NOTE: -O (or --remote-name) is not needed as we already have an /absolute/ file path.
        args.emplace_back("-o");
        args.emplace_back(targetFilePath);

        args.emplace_back("-m"); // -m, --max-time <seconds>
        args.emplace_back("3600"); // 1 hour

        // Finally, tack the url onto the end
        args.emplace_back(url);

        std::string joined_args;
        for (const auto& arg : args)
        {
            if (!joined_args.empty()) {
                joined_args += " ";
            }
            joined_args += arg;
        }
        Log_Info("Using cmdline: /usr/bin/curl %s", joined_args.c_str());

        std::string output;
        int exitCode = ADUC_LaunchChildProcess("/usr/bin/curl", args, output);
        if (exitCode == 0)
        {
            result.ResultCode = ADUC_Result_Download_Success;
            Log_Info("Download output:: \n%s", output.c_str());
        }
        else
        {
            result.ResultCode = ADUC_Result_Failure;
            result.ExtendedResultCode = ADUC_ERROR_CURL_DOWNLOADER_EXTERNAL_FAILURE(exitCode);
            Log_Error("Curl process error, exitCode: %d\nDownload output: \n%s\n", exitCode, output.c_str());
        }
    }
    catch (...)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_DOWNLOAD_EXCEPTION;
    }

    Log_Info("RootKey Package Download with Curl - rc: %d, erc: 0x%08x", result.ResultCode, result.ExtendedResultCode);

    return result;
}

EXTERN_C_END
