/**
 * @file linux_adu_core_exports.cpp
 * @brief Implements exported methods for platform-specific ADUC agent code.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/adu_core_exports.h"
#include "aduc/c_utils.h"
#include "aduc/config_utils.h" // ADUC_ConfigInfo*
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "aduc/shutdown_service.h"
#include "errno.h"
#include "linux_adu_core_impl.hpp"
#include <memory>
#include <string>
#include <vector>

EXTERN_C_BEGIN

/**
 * @brief Register this platform layer and appropriate callbacks for all update actions.
 *
 * @param data Information about this module (e.g. callback methods)
 * @return ADUC_Result Result code.
 */
ADUC_Result ADUC_RegisterPlatformLayer(ADUC_UpdateActionCallbacks* data, int /*argc*/, const char** /*argv*/)
{
    try
    {
        std::unique_ptr<ADUC::LinuxPlatformLayer> pImpl{ ADUC::LinuxPlatformLayer::Create() };
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
    ADUC::LinuxPlatformLayer* pImpl{ static_cast<ADUC::LinuxPlatformLayer*>(token) };
    delete pImpl; // NOLINT(cppcoreguidelines-owning-memory)
}

/**
 * @brief Reboot the system.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_RebootSystem()
{
    int exitStatus = 0;
    std::string output;
    Log_Info("ADUC_RebootSystem called. Rebooting system.");

    // Commit buffer cache to disk.
    sync();

    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        Log_Error("Failed to get config info instance.");
        exitStatus = ENOMEM;
    }
    else
    {
        const std::vector<std::string> args = { "--config-folder", config->configFolder, "--update-type",
                                                "common",          "--update-action",    "reboot" };

        exitStatus = ADUC_LaunchChildProcess(config->aduShellFilePath, args, output);
    }

    if (exitStatus != 0)
    {
        Log_Error("Reboot failed. Process exit with code: %d", exitStatus);
    }

    if (!output.empty())
    {
        Log_Info(output.c_str());
    }

    ADUC_ConfigInfo_ReleaseInstance(config);
    return exitStatus;
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
