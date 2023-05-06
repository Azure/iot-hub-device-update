# coding=utf-8
# --------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for license information.
# --------------------------------------------------------------------------
#
# These are the definitions used by the scenario to complete it's operations. It is used by all the TestCases held within
# the project.
#
import os
import sys
import uuid

sys.path.append('./')
from testingtoolkit import UpdateId

#
# Base Device Definitions
#

class DuScenarioDefinitionManager:
    def __init__(self):
        self.test_device_id = ""
        self.test_adu_group = ""
        self.test_apt_deployment_id = str(uuid.uuid4())
        self.test_operation_id = str(uuid.uuid4()).replace('-', '')
        self.test_mcu_deployment_id = str(uuid.uuid4())
        self.test_bundle_update_deployment_id = str(uuid.uuid4())
        self.test_result_file_prefix = ''
        self.test_connection_timeout_tries = 10
# For all retries this is the total amount of time we wait for all operations
        self.retry_wait_time_in_seconds = 60

    @classmethod
    def FromOSEnvironment(cls):
        newCls = cls()
        newCls.ParseFromOsEnviron()
        return newCls

    def ParseFromOsEnviron(self):
        distro_name = os.environ.get("DISTRONAME", "ubuntu-20.04-amd64")  # Default to "ubuntu-20.04-amd64" if not set

        distro, version, architecture = distro_name.split('-')
        distro_version_name = distro.capitalize().replace('.', '') + version

        self.test_device_id = f"{distro_name}-deployment-test-device"
        self.test_adu_group = f"{distro_version_name}{architecture.upper()}TestGroup"
        self.test_result_file_prefix = distro_name

    # test_device_id
    @property
    def test_device_id(self):
        return self._test_device_id

    @test_device_id.setter
    def test_device_id(self, value):
        self._test_device_id = value

    # test_adu_group
    @property
    def test_adu_group(self):
        return self._test_adu_group

    @test_adu_group.setter
    def test_adu_group(self, value):
        self._test_adu_group = value

    # test_apt_deployment_id
    @property
    def test_apt_deployment_id(self):
        return self._test_apt_deployment_id

    @test_apt_deployment_id.setter
    def test_apt_deployment_id(self, value):
        self._test_apt_deployment_id = value

    # test_operation_id
    @property
    def test_operation_id(self):
        return self._test_operation_id

    @test_operation_id.setter
    def test_operation_id(self, value):
        self._test_operation_id = value

    # test_mcu_deployment_id
    @property
    def test_mcu_deployment_id(self):
        return self._test_mcu_deployment_id

    @test_mcu_deployment_id.setter
    def test_mcu_deployment_id(self, value):
        self._test_mcu_deployment_id = value

    # test_bundle_update_deployment_id
    @property
    def test_bundle_update_deployment_id(self):
        return self._test_bundle_update_deployment_id

    @test_bundle_update_deployment_id.setter
    def test_bundle_update_deployment_id(self, value):
        self._test_bundle_update_deployment_id = value

    # test_result_file_prefix
    @property
    def test_result_file_prefix(self):
        return self._test_result_file_prefix

    @test_result_file_prefix.setter
    def test_result_file_prefix(self, value):
        self._test_result_file_prefix = value

    # test_connection_timeout_tries
    @property
    def test_connection_timeout_tries(self):
        return self._test_connection_timeout_tries

    @test_connection_timeout_tries.setter
    def test_connection_timeout_tries(self, value):
        self._test_connection_timeout_tries = value


#
# Other variables that should be available to all tests in this scenario may be added here
#
