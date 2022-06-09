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
from xmlrunner.extra.xunit_plugin import transform
# Note: the intention is that this script is called like:
# python ./scenarios/<scenario-name>/testscript.py
sys.path.append('./scenarios/')
from testingtoolkit import DeviceUpdateTestHelper
from testingtoolkit import UpdateId
from testingtoolkit import DeploymentStatusResponse
from testingtoolkit import DuAutomatedTestConfigurationManager

sys.path.append('./scenarios/ubuntu-18.04-amd64/')
from scenario_definitions import test_device_id, test_adu_group, test_result_file_prefix

class AddDeviceToGroupTest(unittest.TestCase):

    def test_AddDeviceToGroup(self):
        self.aduTestConfig = DuAutomatedTestConfigurationManager.FromOSEnvironment()
        self.duTestHelper = self.aduTestConfig.CreateDeviceUpdateTestHelper()
        #
        # Before anything else we need to wait and check the device connection status
        # We expect the device to connect within 3 minutes of setting up the device in the step previous
        #
        connectionStatus = ""
        for i in range(0, 5):
            connectionStatus = self.duTestHelper.GetConnectionStatusForDevice(test_device_id)
            if (connectionStatus == "Connected"):
                break
            time.sleep(30)

        self.assertEqual(connectionStatus, "Connected")

        #
        # Before we can run the deployment we need to add the ADUGroup test_adu_group tag to the
        # the device before we make the ADUGroup which we can then use to target the deployment
        # to the device.
        #
        success = self.duTestHelper.AddDeviceToGroup(test_device_id, test_adu_group)

        self.assertTrue(success)

        time.sleep(60)

        deviceClassId = self.duTestHelper.GetAduDeviceClassIdForDevice(test_device_id)

        self.assertNotEqual(deviceClassId, "")

        #
        # Create the ADUGroup
        #
        status_code = self.duTestHelper.CreateADUGroup(test_adu_group, deviceClassId)

        self.assertEqual(status_code, 200)


if (__name__ == "__main__"):
    out = io.BytesIO()

    unittest.main(testRunner=xmlrunner.XMLTestRunner(output=out),
                  failfast=False, buffer=False, catchbreak=False, exit=False)

    with open('./testresults/' + test_result_file_prefix + 'add-device-to-adu-group-test.xml', 'wb') as report:
        report.write(transform(out.getvalue()))
