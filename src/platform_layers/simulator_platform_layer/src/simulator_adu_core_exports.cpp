/**
 * @file simulator_adu_core_exports.cpp
 * @brief Implements exported methods for platform-specific ADUC agent code.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "simulator_adu_core_impl.hpp"
#include "simulator_device_info.h"

#include <ostream>
#include <unordered_map>

#include <aduc/adu_core_exports.h>
#include <aduc/logging.h>

/**
 * @brief Convert SimulationType to string for an ostream.
 *
 * @param strm ostream to write to.
 * @param type Value to convert.
 * @return ostream& @p strm.
 */

std::ostream& operator<<(std::ostream& strm, const SimulationType& type)
{
    switch (type)
    {
    case SimulationType::DownloadFailed:
        strm << "DownloadFailed";
        break;
    case SimulationType::InstallationFailed:
        strm << "InstallationFailed";
        break;
    case SimulationType::ApplyFailed:
        strm << "ApplyFailed";
        break;
    case SimulationType::IsInstalledFailed:
        strm << "IsInstalledFailed";
        break;
    case SimulationType::AllSuccessful:
        strm << "AllSuccessful";
        break;
    }

    return strm;
}

EXTERN_C_BEGIN

/**
 * @brief Register this module for callbacks.
 *
 * @param data Information about this module (e.g. callback methods)
 * @param argc Count of optional arguments.
 * @param argv Initialization arguments, size is argc.
 * @return ADUC_Result Result code.
 */
ADUC_Result ADUC_RegisterPlatformLayer(ADUC_UpdateActionCallbacks* data, unsigned int argc, const char** argv)
{
    try
    {
        // simulation_mode= argument.
        const std::string simulationModeArgPrefix{ "simulation_mode=" };
        const std::unordered_map<std::string, SimulationType> simulationMap{
            { "downloadfailed", SimulationType::DownloadFailed },
            { "installationfailed", SimulationType::InstallationFailed },
            { "applyfailed", SimulationType::ApplyFailed },
            { "isinstalledfailed", SimulationType::IsInstalledFailed },
            { "allsuccessful", SimulationType::AllSuccessful },
        };
        SimulationType simulationType = SimulationType::AllSuccessful;

        const std::string manufacturerArgPrefix{ "deviceinfo_manufacturer=" };
        const std::string modelArgPrefix{ "deviceinfo_model=" };
        const std::string swVersionArgPrefix{ "deviceinfo_swversion=" };

        for (unsigned index = 0; index < argc; ++index)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            const std::string argument{ argv[index] };
            const unsigned int dashdash_cch = 2; // 2 character in "--"

            if (argument.substr(0, dashdash_cch) != "--")
            {
                continue;
            }

            if (argument.substr(dashdash_cch, manufacturerArgPrefix.size()) == manufacturerArgPrefix)
            {
                const std::string value{ argument.substr(dashdash_cch + manufacturerArgPrefix.size()) };
                Log_Info("[Args] Using DeviceInfo manufacturer %s", value.c_str());
                Simulator_DeviceInfo_SetManufacturer(value.c_str());
            }
            else if (argument.substr(dashdash_cch, modelArgPrefix.size()) == modelArgPrefix)
            {
                const std::string value{ argument.substr(dashdash_cch + modelArgPrefix.size()) };
                Log_Info("[Args] Using DeviceInfo model %s", value.c_str());
                Simulator_DeviceInfo_SetModel(value.c_str());
            }
            else if (argument.substr(dashdash_cch, swVersionArgPrefix.size()) == swVersionArgPrefix)
            {
                const std::string value{ argument.substr(dashdash_cch + swVersionArgPrefix.size()) };
                Log_Info("[Args] Using DeviceInfo swversion %s", value.c_str());
                Simulator_DeviceInfo_SetSwVersion(value.c_str());
            }
            else if (argument.substr(dashdash_cch, simulationModeArgPrefix.size()) == simulationModeArgPrefix)
            {
                const std::string value{ argument.substr(dashdash_cch + simulationModeArgPrefix.size()) };
                if (value.empty())
                {
                    continue;
                }

                try
                {
                    simulationType = simulationMap.at(value);
                }
                catch (std::out_of_range&)
                {
                    Log_Error("[Args] Invalid simulation mode %s", value.c_str());
                    throw;
                }

                Log_Info("[Args] Using simulation mode %s", value.c_str());
            }
        }

        std::unique_ptr<ADUC::SimulatorPlatformLayer> pImpl{ ADUC::SimulatorPlatformLayer::Create(
            simulationType) };
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
    ADUC::SimulatorPlatformLayer* pImpl{ static_cast<ADUC::SimulatorPlatformLayer*>(token) };
    delete pImpl; // NOLINT(cppcoreguidelines-owning-memory)
}

/**
 * @brief Reboot the system.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_RebootSystem()
{
    Log_Info("ADUC_RebootSystem called.");

    // Here's where the system would be rebooted.
    return 0;
}

/**
 * @brief Restart the ADU Agent.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_RestartAgent()
{
    Log_Info("ADUC_RestartAgent called.");

    // Here's where the agent would be restarted.
    return 0;
}

EXTERN_C_END
