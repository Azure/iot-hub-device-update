/**
 * @file adps2mqtt_client_module_ft.cpp
 * @brief Azure DPS MQTT Client module functional tests
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adps2_mqtt_client_module.h"
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/system_utils.h"
#include "du_agent_sdk/agent_module_interface.h"

#include <aducpal/unistd.h>
#include <signal.h>

static void set_test_config_folder()
{
    setenv(ADUC_CONFIG_FOLDER_ENV, "/tmp/adu/testdata/adps2mqtt-client-module-test-data", 1);
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

    ADUC_AGENT_MODULE_HANDLE handle = ADPS_MQTT_Client_Module_Create();
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)handle;

    // Register signal handler function so that we can exit cleanly
    // SGININT is defined in signal.h and is the signal sent by Ctrl-C
    signal(SIGINT, _SignalHandler);

    // Initialize the module
    int result = interface->initializeModule(handle, nullptr);
    keep_running_ft = true;

    while (keep_running_ft)
    {
        // Call do_work
        result = interface->doWork(handle);

        // Sleep for 200ms
        usleep(100000);
    }

    // Deinitialize the module
    result = interface->deinitializeModule(handle);

    // Destroy the module
    ADPS_MQTT_Client_Module_Destroy(handle);
}
