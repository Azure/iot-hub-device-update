/**
 * @file microsoft_delta_download_handler_utils.c
 * @brief The Microsoft delta download handler helper function implementations.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/microsoft_delta_download_handler_utils.h"
#include "aduc/c_utils.h" // EXTERN_C_BEGIN, EXTERN_C_END
#include "aduc/logging.h"
#include "aduc/result.h" // MAKE_DELTA_PROCESSOR_EXTENDEDRESULTCODE
#include "aduc/shared_lib.hpp"

const char* AduDiffSharedLibName = "libadudiffapi.so";

// function pointer types
using adu_apply_handle = void*;

using adu_diff_apply_create_session_fn = adu_apply_handle (*)();
using adu_diff_apply_close_session_fn = void (*)(adu_apply_handle handle);

using adu_diff_apply_fn =
    int (*)(adu_apply_handle session, const char* sourcePath, const char* deltaPath, const char* targetPath);

using adu_diff_apply_get_error_count_fn = size_t (*)(adu_apply_handle handle);
using adu_diff_apply_get_error_text_fn = const char* (*)(adu_apply_handle handle, size_t index);
using adu_diff_apply_get_error_code_fn = int (*)(adu_apply_handle handle, size_t index);

EXTERN_C_BEGIN

/**
 * @brief Creates a target update from the source and delta updates.
 *
 * @param sourceUpdateFilePath The source update path.
 * @param deltaUpdateFilePath The delta update path.
 * @param targetUpdatePath The target update path.
 * @return ADUC_Result The result.
 */
ADUC_Result MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate(
    const char* sourceUpdateFilePath, const char* deltaUpdateFilePath, const char* targetUpdateFilePath)
{
    Log_Debug(
        "Making '%s' from src '%s' and delta '%s'",
        targetUpdateFilePath,
        sourceUpdateFilePath,
        deltaUpdateFilePath);

    ADUC_Result result = { ADUC_Result_Failure };

    adu_apply_handle session = nullptr;

    adu_diff_apply_create_session_fn createSessionFn = nullptr;
    adu_diff_apply_close_session_fn closeSessionFn = nullptr;
    adu_diff_apply_fn applyFn = nullptr;
    adu_diff_apply_get_error_count_fn getErrorCountFn = nullptr;
    adu_diff_apply_get_error_text_fn getErrorTextFn = nullptr;
    adu_diff_apply_get_error_code_fn getErrorCodeFn = nullptr;

    try
    {
        Log_Debug("load diff processor %s ...", AduDiffSharedLibName);

        result.ExtendedResultCode = ADUC_ERC_DDH_PROCESSOR_LOAD_LIB;
        aduc::SharedLib diffApi{ AduDiffSharedLibName };

        Log_Debug("ensure symbols ...");

        result.ExtendedResultCode = ADUC_ERC_DDH_PROCESSOR_ENSURE_SYMBOLS;
        diffApi.EnsureSymbols({ "adu_diff_apply",
                                "adu_diff_apply_close_session",
                                "adu_diff_apply_create_session",
                                "adu_diff_apply_get_error_code",
                                "adu_diff_apply_get_error_count",
                                "adu_diff_apply_get_error_text" });

        createSessionFn =
            reinterpret_cast<adu_diff_apply_create_session_fn>(diffApi.GetSymbol("adu_diff_apply_create_session"));
        closeSessionFn =
            reinterpret_cast<adu_diff_apply_close_session_fn>(diffApi.GetSymbol("adu_diff_apply_close_session"));
        applyFn = reinterpret_cast<adu_diff_apply_fn>(diffApi.GetSymbol("adu_diff_apply"));
        getErrorCountFn =
            reinterpret_cast<adu_diff_apply_get_error_count_fn>(diffApi.GetSymbol("adu_diff_apply_get_error_count"));
        getErrorTextFn =
            reinterpret_cast<adu_diff_apply_get_error_text_fn>(diffApi.GetSymbol("adu_diff_apply_get_error_text"));
        getErrorCodeFn =
            reinterpret_cast<adu_diff_apply_get_error_code_fn>(diffApi.GetSymbol("adu_diff_apply_get_error_code"));

        Log_Debug("create session ...");

        session = createSessionFn();
        if (session == nullptr)
        {
            Log_Error("create diffapply session failed");
            result.ExtendedResultCode = ADUC_ERC_DDH_PROCESSOR_CREATE_SESSION;
        }
        else
        {
            Log_Debug("Apply diff ...");

            int res = applyFn(session, sourceUpdateFilePath, deltaUpdateFilePath, targetUpdateFilePath);

            if (res == 0)
            {
                result.ResultCode = ADUC_Result_Success;
            }
            else
            {
                Log_Error("diff apply - overall err: %d", res);
                result.ExtendedResultCode = MAKE_DELTA_PROCESSOR_EXTENDEDRESULTCODE(res);

                size_t errorCount = getErrorCountFn(session);
                for (size_t errIndex = 0; errIndex < errorCount; ++errIndex)
                {
                    int error_code = getErrorCodeFn(session, errIndex);
                    const char* error_text = getErrorTextFn(session, errIndex); // do not free
                        //
                    Log_Error("diff apply - errcode %d: '%s'", error_code, error_text);

                    result.ExtendedResultCode = MAKE_DELTA_PROCESSOR_EXTENDEDRESULTCODE(error_code);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        Log_Error("Unhandled std exception: %s", e.what());
    }
    catch (...)
    {
        Log_Error("Unhandled exception");
    }

    if (session != nullptr && closeSessionFn != nullptr)
    {
        Log_Debug("close session ...");
        closeSessionFn(session);
    }

    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        result.ExtendedResultCode = 0;
    }

    Log_Debug("ResultCode %d, erc %d", result.ResultCode, result.ExtendedResultCode);

    return result;
}

EXTERN_C_END
