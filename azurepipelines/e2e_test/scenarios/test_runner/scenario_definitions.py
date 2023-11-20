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
        self.test_apt_fails_deployment_id = str(uuid.uuid4())
        self.test_operation_id = str(uuid.uuid4()).replace('-', '')
        self.test_mcu_deployment_id = str(uuid.uuid4())
        self.test_bundle_update_deployment_id = str(uuid.uuid4())
        self.test_result_file_prefix = ''
        self.test_connection_timeout_tries = 10
# For all retries this is the total amount of time we wait for all operations
        self.retry_wait_time_in_seconds = 60
        self.config_method = ''

    @classmethod
    def FromOSEnvironment(cls):
        newCls = cls()
        newCls.ParseFromOsEnviron()
        return newCls

    def ParseFromOsEnviron(self):
        distro_name = os.environ.get("DISTRONAME", "ubuntu-20.04-amd64")  # Default to "ubuntu-20.04-amd64" if not set
        device_id = os.environ.get("DEVICEID", "")

        if (device_id == "" or distro_name == ""):
            print("DEVICEID or DISTRONAME is not set in the environment")
            sys.exit(1)

        distro, version, architecture = distro_name.split('-')
        distro_version_name = distro.capitalize().replace('.', '') + version

        self.test_device_id = device_id
        self.test_adu_group = f"{distro_version_name}{architecture.upper()}TestGroup"
        self.test_result_file_prefix = distro_name
        if distro_name == "debian-10-amd64":
            self.config_method = "string"
        else:
            self.config_method = "AIS"


#
# Other variables that should be available to all tests in this scenario may be added here
#
