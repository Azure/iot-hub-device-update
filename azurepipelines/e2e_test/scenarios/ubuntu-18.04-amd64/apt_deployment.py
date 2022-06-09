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
# python ./scenarios/<scenario-name>/testscript.py
sys.path.append('./scenarios/')
from testingtoolkit import DeviceUpdateTestHelper
from testingtoolkit import UpdateId
from testingtoolkit import DeploymentStatusResponse
from testingtoolkit import DuAutomatedTestConfigurationManager
from xmlrunner.extra.xunit_plugin import transform

sys.path.append('./scenarios/ubuntu-18.04-amd64/')
from scenario_definitions import test_device_id, test_adu_group, test_result_file_prefix, test_apt_deployment_id

class AptDeploymentTest(unittest.TestCase):

    def test_AptDeployment(self):
        self.aduTestConfig = DuAutomatedTestConfigurationManager.FromOSEnvironment()
        self.duTestHelper = self.aduTestConfig.CreateDeviceUpdateTestHelper()

        #
        # These are the defined values that will be used to run the tests
        # we expect the device to have already been created by the devicesetup.py
        # script that generated the configuration that will be used on the device
        #
        self.deploymentId = test_apt_deployment_id

        self.deploymentUpdateId = UpdateId(provider="Fabrikaam", name="Vacuum", version="1.0.2")

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
        # The assumption is that we've already imported the update targeting the manufacturer and
        # model for the device created in the devicesetup.py. Now we have to create the deployment for
        # the device using the update-id for the update
        #
        status_code = self.duTestHelper.StartDeploymentForGroup(deploymentId=self.deploymentId, groupName=test_adu_group, updateId=self.deploymentUpdateId)

        self.assertEqual(status_code, 200)

        #
        # Once we've started the deployment for the group we can then query for the status of the deployment
        # to see if/when the deployment finishes. We expect a timeout of 10 minutes
        #
        deploymentStatus = None

        for i in range(0, 15):
            deploymentStatus = self.duTestHelper.GetDeploymentStatusForGroup(self.deploymentId, test_adu_group)

            #
            # If we see all the devices completing the deployment
            if (deploymentStatus.devicesCompletedSucceededCount != 0 and deploymentStatus.totalDevices != 0):
                # Check the status of the devices within the deployment
                break
            time.sleep(60)

        self.assertEqual(deploymentStatus.totalDevices, deploymentStatus.devicesCompletedSucceededCount)

        # Sleep to give time for changes to propogate
        time.sleep(30)

        #
        # Once we've checked that we've reached the proper outcome for the
        # deployment we need to check that the device itself has reported it
        # is back in the idle state.
        #
        twin = self.duTestHelper.GetDeviceTwinForDevice(test_device_id)

        self.assertEqual(twin.properties.reported["deviceUpdate"]["agent"]["state"], 0)

        #
        # In case of a succeeded deployment we need to clean up the resources we created.
        # mainly the deletion of the deployment
        #
        time.sleep(60)

        if (deploymentStatus.devicesInProgressCount != 0):
            self.assertEqual(self.duTestHelper.StopDeployment(self.deploymentId, test_adu_group), 200)
            time.sleep(60)

        # Once stopped we can delete the deployment
        self.assertEqual(self.duTestHelper.DeleteDeployment(self.deploymentId, test_adu_group), 204)


if (__name__ == "__main__"):
    out = io.BytesIO()

    unittest.main(testRunner=xmlrunner.XMLTestRunner(output=out),
                  failfast=False, buffer=False, catchbreak=False, exit=False)

    with open('./testresults/' + test_result_file_prefix + '-apt-deployment-test.xml', 'wb') as report:
        report.write(transform(out.getvalue()))
