/**
 * @file adu_mqtt_client_module_ft.cpp
 * @brief Implements functional tests for the Device Update MQTT client module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "du_agent_sdk/agent_module_interface.h"
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/system_utils.h"

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

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

extern ADUC_AGENT_MODULE_INTERFACE ADUC_MQTT_Client_ModuleInterface;

TEST_CASE("EGBroker Device Enrollment")
{
    set_test_config_folder();

    ADUC_AGENT_MODULE_HANDLE handle = ADUC_MQTT_Client_ModuleInterface.create();

    // Register signal handler function so that we can exit cleanly
    // SGININT is defined in signal.h and is the signal sent by Ctrl-C
    signal(SIGINT, _SignalHandler);

    CHECK(handle != nullptr);

    // Initialize the module
    int result = ADUC_MQTT_Client_ModuleInterface.initializeModule(handle, nullptr);
    CHECK(result == 0);


    keep_running_ft = true;

    while (keep_running_ft)
    {
        // Call do_work
            result = ADUC_MQTT_Client_ModuleInterface.doWork(handle);

        // Sleep for 200ms
        usleep(100000);
    }

    // Deinitialize the module
    result = ADUC_MQTT_Client_ModuleInterface.deinitializeModule(handle);

    CHECK(result == 0);

    // Destroy the module
    ADUC_MQTT_Client_ModuleInterface.destroy(handle);

}
