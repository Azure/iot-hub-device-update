# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
import io
import sys
import time
import unittest
import xmlrunner

# Note: the intention is that this script is called like:
# python ./scenarios/test_runner/testscript.py
sys.path.append('./scenarios/')
from testingtoolkit import DeviceUpdateTestHelper
from testingtoolkit import UpdateId
from testingtoolkit import DeploymentStatusResponse
from testingtoolkit import DuAutomatedTestConfigurationManager
from testingtoolkit import DiagnosticLogCollectionStatusResponse
from xmlrunner.extra.xunit_plugin import transform

# Note: the intention is that this script is called like:
# python ./scenarios/test_runner/<test-script-name>.py
sys.path.append('./scenarios/test_runner/')
from scenario_definitions import DuScenarioDefinitionManager


diagnostics_operation_status_retries = 15
test_result_file_prefix=""

class DiagnosticsTest(unittest.TestCase):
    def test_diagnostics(self):
        self.aduTestConfig = DuAutomatedTestConfigurationManager.FromOSEnvironment()
        self.duTestHelper = self.aduTestConfig.CreateDeviceUpdateTestHelper()

        self.aduScenarioDefinition = DuScenarioDefinitionManager.FromOSEnvironment()

        test_device_id = self.aduScenarioDefinition.test_device_id
        global test_result_file_prefix
        test_result_file_prefix = self.aduScenarioDefinition.test_result_file_prefix
        test_operation_id = self.aduScenarioDefinition.test_operation_id
        test_connection_timeout_tries = self.aduScenarioDefinition.test_connection_timeout_tries
        retry_wait_time_in_seconds = self.aduScenarioDefinition.retry_wait_time_in_seconds
        config_method = self.aduScenarioDefinition.config_method

        #
        # We retrieve the test operation id to be used by the script from the scenario definitions file. It's important to keep
        # things like the deployment id, device-id, module-id, and other scenario level definitions that might effect other
        # tests in the scenario_definitions.py file.
        #
        self.operationId = test_operation_id

        #
        # Before anything else we need to wait and check the device connection status
        # We expect the device to connect within the configured amount of time of setting up the device in the step previous
        #
        connectionStatus = ""
        for i in range(0, 5):
            if config_method == "AIS":
                connectionStatus = self.duTestHelper.GetConnectionStatusForModule(test_device_id, "IoTHubDeviceUpdate")
            else:
                connectionStatus = self.duTestHelper.GetConnectionStatusForDevice(test_device_id)

            if (connectionStatus == "Connected"):
                break
            time.sleep(30)

        self.assertEqual(connectionStatus, "Connected")

        #
        # The assumption is that we've already configured diagnostics in test account and already run a apt deployment
        # Start a diagnostics log collection operation
        # 201 indicate the device diagnostics log collection operation is created
        #

        if config_method == "AIS":
            diagnosticResponseStatus_code = self.duTestHelper.RunDiagnosticsOnDeviceOrModule(
                test_device_id, operationId=self.operationId, moduleId="IoTHubDeviceUpdate", description="")
        else:
            diagnosticResponseStatus_code = self.duTestHelper.RunDiagnosticsOnDeviceOrModule(
            test_device_id, operationId=self.operationId, description="")

        self.assertEqual(diagnosticResponseStatus_code, 201)

        #
        # Get device diagnostics log collection operatio status and device status
        # device status represent Log upload status, operatin status represent background operation status
        #
        diagnosticJsonStatus = DiagnosticLogCollectionStatusResponse()

        for i in range(0, diagnostics_operation_status_retries):
            time.sleep(5)
            diagnosticJsonStatus = self.duTestHelper.GetDiagnosticsLogCollectionStatus(
                self.operationId)

            if (diagnosticJsonStatus.status == "Succeeded"):
                break

        self.assertEqual(diagnosticJsonStatus.status, "Succeeded")

        self.assertEqual(len(diagnosticJsonStatus.deviceStatus), 1)
        self.assertEqual(
            diagnosticJsonStatus.deviceStatus[0].status, "Succeeded")
        self.assertEqual(
            diagnosticJsonStatus.deviceStatus[0].resultCode, "200")


#
# Below is the section of code that uses the above class to run the test. It starts by running the test, capturing the output, transforming
# the output from Python Unittest to X/JUnit format. Then the function exports the values to an xml file in the testresults directory which is
# then uploaded by the Azure Pipelines PostTestResults job
#
if (__name__ == "__main__"):
    out = io.BytesIO()

    result = unittest.main(testRunner=xmlrunner.XMLTestRunner(output=out),
                  failfast=False, buffer=False, catchbreak=False, exit=False)

    with open('./testresults/' + test_result_file_prefix + '-diagnostic-test.xml', 'wb') as report:
        report.write(transform(out.getvalue()))

    if (len(result.failures) > 0):
        sys.exit(1)
