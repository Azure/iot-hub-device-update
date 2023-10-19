/**
 * @file adu_mqtt_client_module_ft.cpp
 * @brief Implements functional tests for the Device Update MQTT client module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_mqtt_client_module.h"
#include "aduc/agent_state_store.h"
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/system_utils.h"
#include "du_agent_sdk/agent_module_interface.h"

#include <aducpal/unistd.h>
#include <signal.h>
#include <sstream>
#include <string>

static void set_test_config_folder()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/adu-mqtt-client-module-test-data";
    setenv(ADUC_CONFIG_FOLDER_ENV, path.c_str(), 1);
}

bool keep_running_ft = true;

// Signal handler for Ctrl-C
static void _SignalHandler(int signum)
{
    (void)signum;
    keep_running_ft = 0;
}

int main()
{
    set_test_config_folder();
    // Need to set ret and goto done after this to ensure proper shutdown and deinitialization.
    ADUC_Logging_Init(ADUC_LOG_DEBUG, "du-mqtt-client-module-ft");

    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/adu-mqtt-client-module-test-data";
    path += "/aduc_state_store.json";

    ADUC_StateStore_Initialize(path.c_str());

    ADUC_AGENT_MODULE_HANDLE duClientHandle = ADUC_MQTT_Client_Module_Create();
    if (duClientHandle == nullptr)
    {
        Log_Error("Failed to create module handle");
        return -1;
    }
    ADUC_AGENT_MODULE_INTERFACE* duClientInterface = (ADUC_AGENT_MODULE_INTERFACE*)duClientHandle;

    // Register signal handler function so that we can exit cleanly
    // SGININT is defined in signal.h and is the signal sent by Ctrl-C
    signal(SIGINT, _SignalHandler);

    // Initialize the module
    int result = duClientInterface->initializeModule(duClientHandle, nullptr);
    if (result != 0)
    {
        Log_Error("Failed to initialize DU MQTT client module");
        return -1;
    }

    keep_running_ft = true;

    while (keep_running_ft)
    {
        // Call do_work
        duClientInterface->doWork(duClientHandle);

        usleep(100000);
    }

    // Deinitialize the module
    result = duClientInterface->deinitializeModule(duClientHandle);
    if (result != 0)
    {
        Log_Error("Failed to deinitialize DU MQTT client module");
        return -1;
    }

    // Destroy the module
    ADUC_MQTT_Client_Module_Destroy(duClientHandle);

    ADUC_Logging_Uninit();
    return 0;
}
