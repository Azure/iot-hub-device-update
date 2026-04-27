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

// Default timeout values for curl downloads (in seconds).
// TODO: Make these configurable via du-config.json and/or compile-time variables.
#ifndef ADUC_CURL_CONNECT_TIMEOUT_SECS
#define ADUC_CURL_CONNECT_TIMEOUT_SECS 30
#endif

#ifndef ADUC_CURL_MAX_TIME_SECS
#define ADUC_CURL_MAX_TIME_SECS 3600
#endif

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

        // --connect-timeout <seconds>. Maximum time for connection phase (DNS + TCP handshake).
        // Prevents long waits on unreachable hosts or invalid URLs.
        args.emplace_back("--connect-timeout");
        args.emplace_back(std::to_string(ADUC_CURL_CONNECT_TIMEOUT_SECS));

        // -m, --max-time <seconds>. Maximum total time for the entire operation.
        args.emplace_back("-m");
        args.emplace_back(std::to_string(ADUC_CURL_MAX_TIME_SECS));

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
            Log_Info("Download succeeded for '%s'", url);
        }
        else
        {
            result.ResultCode = ADUC_Result_Failure;
            result.ExtendedResultCode = ADUC_ERROR_CURL_DOWNLOADER_EXTERNAL_FAILURE(exitCode);

            // curl exit codes: 6=couldn't resolve host, 7=failed to connect,
            // 28=operation timed out, 35=SSL connect error
            if (exitCode == 28)
            {
                Log_Error(
                    "Curl download timed out for '%s' (connect-timeout: %ds, max-time: %ds). Output: %s",
                    url, ADUC_CURL_CONNECT_TIMEOUT_SECS, ADUC_CURL_MAX_TIME_SECS, output.c_str());
            }
            else if (exitCode == 6)
            {
                Log_Error("Curl could not resolve host for '%s'. Output: %s", url, output.c_str());
            }
            else if (exitCode == 7)
            {
                Log_Error("Curl failed to connect to host for '%s'. Output: %s", url, output.c_str());
            }
            else
            {
                Log_Error("Curl download failed for '%s', exitCode: %d. Output: %s", url, exitCode, output.c_str());
            }
        }
    }
    catch (...)
    {
        Log_Error("Exception during curl download of rootkey package, ERC: 0x%08x", ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_DOWNLOAD_EXCEPTION);
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_DOWNLOAD_EXCEPTION;
    }

    Log_Info("RootKey Package Download with Curl - rc: %d, erc: 0x%08x", result.ResultCode, result.ExtendedResultCode);

    return result;
}

EXTERN_C_END
