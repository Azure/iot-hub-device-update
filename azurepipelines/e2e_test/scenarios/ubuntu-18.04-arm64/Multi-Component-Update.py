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

# Note: the intention is that this script is called like:
# python ./scenarios/<scenario-name>/<test-script-name>.py
sys.path.append('./scenarios/ubuntu-18.04-arm64/')
from scenario_definitions import test_device_id, test_adu_group, test_result_file_prefix, test_mcu_deployment_id, test_connection_timeout_tries, retry_wait_time_in_seconds

#
# Global Test Variables
#

mcu_deployment_status_retries = 15

class AptDeploymentTest(unittest.TestCase):

    #
    # Every test within a subclass of a unittest.TestCase to be run by unittest must have the prefix test_
    #
    def test_AptDeployment(self):

        #
        # The first step to any test is to create the test helper that allows us to make calls to both the DU account and the
        # IotHub. In the pipeline the parameters to create the test helper are passed to the environment from which we can read them
        # here.
        #
        # For testing it might be convenient to simply define these above and directly instantiate the object or if you're not planning
        # on running the tests as apart of a unittest.TestCase you can simply pass them via the CLI to the python program
        # and create the test helper using the helper method
        # DuAutomatedTestConfigurationManager.FromCliArgs()
        #
        self.aduTestConfig = DuAutomatedTestConfigurationManager.FromOSEnvironment()
        self.duTestHelper = self.aduTestConfig.CreateDeviceUpdateTestHelper()


        #
        # We retrieve the apt deployment id to be used by the script from the scenario definitions file. It's important to keep
        # things like the deployment id, device-id, module-id, and other scenario level definitions that might effect other
        # tests in the scenario_definitions.py file.
        #
        self.deploymentId = test_mcu_deployment_id

        #
        # Before anything else we need to wait and check the device connection status
        # We expect the device to connect within the configured amount of time of setting up the device in the step previous
        #
        connectionStatus = ""
        for i in range(0,test_connection_timeout_tries):
            connectionStatus = self.duTestHelper.GetConnectionStatusForDevice(test_device_id)
            if (connectionStatus == "Connected"):
                break
            time.sleep(retry_wait_time_in_seconds)

        self.assertEqual(connectionStatus,"Connected")
        #
        # For every deployment we need to create an Update Id. For all tests running in the DeviceUpdate for IotHub Test Automation
        # pipeline we use Fabrikaam as a provider and Vaccuum as the name for the update. You'll see the same values supplied as the
        # manufacturer and model in the devicesetup.py script.
        #
        self.deploymentUpdateId = UpdateId(provider="Contoso1", name="Virtual", version="6.0")

        #
        # The assumption is that we've already imported the update targeting the manufacturer and
        # model for the device created in the devicesetup.py. Now we have to create the deployment for
        # the device using the update-id for the update
        #
        # Note: ALL UPDATES SHOULD BE UPLOADED TO THE TEST AUTOMATION HUB NOTHING SHOULD BE IMPORTED AT TEST TIME
        #
        status_code = self.duTestHelper.StartDeploymentForGroup(deploymentId=self.deploymentId,groupName=test_adu_group,updateId=self.deploymentUpdateId)

        self.assertEqual(status_code,200)

        #
        # Once we've started the deployment for the group we can then query for the status of the deployment
        # to see if/when the deployment finishes. We expect a timeout of the configured amount of time
        #
        deploymentStatus = None


        for i in range(0,mcu_deployment_status_retries):
            deploymentStatus = self.duTestHelper.GetDeploymentStatusForGroup(self.deploymentId,test_adu_group)

            #
            # If we see all the devices have completed the deployment then we can exit early
            #
            if (len(deploymentStatus.subgroupStatuses) != 0):
                if (deploymentStatus.subgroupStatuses[0].devicesCompletedSucceededCount == 1):
                    break
            time.sleep(retry_wait_time_in_seconds)


        #
        # Should only be one device group in the deployment
        #
        self.assertEqual(len(deploymentStatus.subgroupStatuses),1)

        #
        # Devices in the group should have succeeded
        #
        self.assertEqual(deploymentStatus.subgroupStatuses[0].totalDevices,deploymentStatus.subgroupStatuses[0].devicesCompletedSucceededCount)

        # Sleep to give time for changes to propagate and for DU to switch it's state back to idle
        time.sleep(retry_wait_time_in_seconds)

        #
        # Once we've checked that we've reached the proper outcome for the
        # deployment we need to check that the device itself has reported it
        # is back in the idle state.
        #
        twin = self.duTestHelper.GetDeviceTwinForDevice(test_device_id)

        self.assertEqual(twin.properties.reported["deviceUpdate"]["agent"]["state"],0)

        #
        # In case of a succeeded deployment we need to clean up the resources we created.
        # mainly the deletion of the deployment
        #
        time.sleep(retry_wait_time_in_seconds)

        # self.assertEqual(self.duTestHelper.StopDeployment(self.deploymentId,test_adu_group),200)
        # time.sleep(retry_wait_time_in_seconds)

        # Once stopped we can delete the deployment
        self.assertEqual(self.duTestHelper.DeleteDeployment(self.deploymentId,test_adu_group),204)


#
# Below is the section of code that uses the above class to run the test. It starts by running the test, capturing the output, transforming
# the output from Python Unittest to X/JUnit format. Then the function exports the values to an xml file in the testresults directory which is
# then uploaded by the Azure Pipelines PostTestResults job
#
if (__name__ == "__main__"):
    #
    # Create the IO pipe
    #
    out = io.BytesIO()

    #
    # Exercise the TestCase and all the tests within it.
    #
    unittest.main(testRunner = xmlrunner.XMLTestRunner(output=out),failfast=False,buffer=False,catchbreak=False,exit=False)

    #
    # Finally transform the output unto the X/JUnit XML file format
    #
    with open('./testresults/' + test_result_file_prefix + '-mcu-deployment-test.xml','wb') as report:
        report.write(transform(out.getvalue()))
