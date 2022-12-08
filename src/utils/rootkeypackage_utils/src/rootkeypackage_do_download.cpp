/**
 * @file rootkeypackage_do_download.cpp
 * @brief Implements delivery optimization download.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/logging.h>
#include <aduc/result.h>
#include <do_download.h>
#include <system_error>

EXTERN_C_BEGIN

ADUC_Result DownloadRootKeyPkg_DO(const char* url, const char* targetFilePath)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };

    Log_Info("Downloading File '%s' to '%s'", url, targetFilePath);

    const std::error_code ret = microsoft::deliveryoptimization::download::download_url_to_path(url, targetFilePath);
    if (ret == std::error_code())
    {
        result.ResultCode = ADUC_GeneralResult_Success;
    }
    else
    {
        // Note: The call to download_url_to_path() does not make use of a cancellation token,
        // so the download can only timeout or hit a fatal error.
        Log_Error(
            "DO error, msg: %s, code: %#08x, timeout? %d",
            ret.message().c_str(),
            ret.value(),
            ret == std::errc::timed_out);

        result.ExtendedResultCode = MAKE_ADUC_DELIVERY_OPTIMIZATION_EXTENDEDRESULTCODE(ret.value());
    }

    Log_Info("Download erc: 0x%08x", result.ExtendedResultCode);

    return result;
}

EXTERN_C_END
