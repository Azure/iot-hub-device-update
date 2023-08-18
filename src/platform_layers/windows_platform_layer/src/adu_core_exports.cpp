/**
 * @file adu_core_exports.cpp
 * @brief Implements exported methods for platform-specific ADUC agent code.
 *
 * Exports "ADUC_RegisterPlatformLayer", "ADUC_Unregister", "ADUC_RebootSystem", "ADUC_RestartAgent"  methods.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/adu_core_exports.h"
#include "adu_core_impl.hpp"
#include "aduc/c_utils.h"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "aduc/shutdown_service.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

EXTERN_C_BEGIN

/**
 * @brief Register this platform layer and approriate callbacks for all update actions.
 *
 * @param data Information about this module (e.g. callback methods)
 * @return ADUC_Result Result code.
 */
ADUC_Result ADUC_RegisterPlatformLayer(ADUC_UpdateActionCallbacks* data, int /*argc*/, const char** /*argv*/)
{
    try
    {
        std::unique_ptr<ADUC::WindowsPlatformLayer> pImpl{ ADUC::WindowsPlatformLayer::Create() };
        ADUC_Result result{ pImpl->SetUpdateActionCallbacks(data) };
        // The platform layer object is now owned by the UpdateActionCallbacks object.
        pImpl.release();
        return result;
    }
    catch (const ADUC::Exception& e)
    {
        Log_Error("Unhandled ADU Agent exception. code: %d, message: %s", e.Code(), e.Message().c_str());
        return ADUC_Result{ ADUC_Result_Failure, e.Code() };
    }
    catch (const std::exception& e)
    {
        Log_Error("Unhandled std exception: %s", e.what());
        return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
    }
    catch (...)
    {
        return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
    }
}

/**
 * @brief Unregister this module.
 *
 * @param token Token that was returned from #ADUC_RegisterPlatformLayer call.
 */
void ADUC_Unregister(ADUC_Token token)
{
    ADUC::WindowsPlatformLayer* pImpl{ static_cast<ADUC::WindowsPlatformLayer*>(token) };
    delete pImpl; // NOLINT(cppcoreguidelines-owning-memory)
}

/**
 * @brief Reboot the system.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_RebootSystem()
{
    Log_Info("ADUC_RebootSystem called. Rebooting system.");

    // Commit buffer cache to disk (No Windows equivalent)
    // sync();

    HANDLE hToken;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        Log_Error("OpenProcessToken failed, err %d", GetLastError());
        return ENOSYS;
    }

    TOKEN_PRIVILEGES tkp;
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
    if (GetLastError() != ERROR_SUCCESS)
    {
        Log_Error("AdjustTokenPrivileges failed, err %d", GetLastError());
        CloseHandle(hToken);
        return ENOSYS;
    }

    if (!ExitWindowsEx(
            EWX_REBOOT,
            SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SERVICEPACK | SHTDN_REASON_FLAG_PLANNED))
    {
        Log_Error("Reboot failed, err %d", GetLastError());
        CloseHandle(hToken);
        return ENOSYS;
    }

    return 0;
}

/**
 * @brief Restart the ADU Agent.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_RestartAgent()
{
    Log_Info("Restarting ADU Agent.");

    ADUC_ShutdownService_RequestShutdown();

    return 0;
}

EXTERN_C_END
