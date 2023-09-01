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
from xmlrunner.extra.xunit_plugin import transform

# Note: the intention is that this script is called like:
# python ./scenarios/test_runner/<test-script-name>.py
sys.path.append('./scenarios/test_runner/')
from scenario_definitions import DuScenarioDefinitionManager

test_result_file_prefix=""

class DeleteDeviceAndGroup(unittest.TestCase):

    def test_DeleteTestDeviceAndGroup(self):
        configManager = DuAutomatedTestConfigurationManager.FromOSEnvironment()

        duTestHelper = configManager.CreateDeviceUpdateTestHelper()

        aduScenarioDefinition = DuScenarioDefinitionManager.FromOSEnvironment()

        test_device_id = aduScenarioDefinition.test_device_id
        test_adu_group = aduScenarioDefinition.test_adu_group
        global test_result_file_prefix
        test_result_file_prefix = aduScenarioDefinition.test_result_file_prefix
        retry_wait_time_in_seconds = aduScenarioDefinition.retry_wait_time_in_seconds

        self.assertTrue(duTestHelper.DeleteDevice(test_device_id))

        time.sleep(retry_wait_time_in_seconds*5)

        self.assertEqual(duTestHelper.DeleteADUGroup(test_adu_group), 204)

if (__name__ == "__main__"):
    out = io.BytesIO()

    unittest.main(testRunner=xmlrunner.XMLTestRunner(output=out),
                  failfast=False, buffer=False, catchbreak=False, exit=False)

    with open('./testresults/' + test_result_file_prefix + '-delete-device-test.xml', 'wb') as report:
        report.write(transform(out.getvalue()))
