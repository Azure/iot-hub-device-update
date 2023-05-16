# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
import sys

sys.path.append('./scenarios/ubuntu-18.04-amd64/')
from scenario_definitions import test_device_id, test_adu_group, test_result_file_prefix, retry_wait_time_in_seconds
from xmlrunner.extra.xunit_plugin import transform

# Note: the intention is that this script is called like:
# python ./scenarios/<scenario-name>/testscript.py
sys.path.append('./scenarios/')
from testingtoolkit import DuAutomatedTestConfigurationManager
from testingtoolkit import DeviceUpdateTestHelper
import io
import time
import unittest
import xmlrunner


class DeleteDeviceAndGroup(unittest.TestCase):

    def test_DeleteTestDeviceAndGroup(self):
        configManager = DuAutomatedTestConfigurationManager.FromOSEnvironment()

        duTestHelper = configManager.CreateDeviceUpdateTestHelper()

        self.assertTrue(duTestHelper.DeleteDevice(test_device_id))

        time.sleep(retry_wait_time_in_seconds*5)

        self.assertEqual(duTestHelper.DeleteADUGroup(test_adu_group), 204)


if (__name__ == "__main__"):
    out = io.BytesIO()

    unittest.main(testRunner=xmlrunner.XMLTestRunner(output=out),
                  failfast=False, buffer=False, catchbreak=False, exit=False)

    with open('./testresults/' + test_result_file_prefix + '-delete-device-test.xml', 'wb') as report:
        report.write(transform(out.getvalue()))
