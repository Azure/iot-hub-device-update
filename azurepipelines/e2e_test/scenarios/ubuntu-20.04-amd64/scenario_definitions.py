# coding=utf-8
# --------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for license information.
# --------------------------------------------------------------------------
#
# These are the definitions used by the scenario to complete it's operations. It is used by all the TestCases held within
# the project.
#
import sys
import uuid

sys.path.append('./')
from testingtoolkit import UpdateId

#
# Base Device Definitions
#
test_device_id = "ubuntu20.04-amd64-deployment-test-device"

test_adu_group = "Ubuntu2004AMD64TestGroup"

test_apt_deployment_id = str(uuid.uuid4())

test_operation_id = str(uuid.uuid4()).replace('-', '')

test_mcu_deployment_id = str(uuid.uuid4())

test_bundle_update_deployment_id = str(uuid.uuid4())

test_result_file_prefix = 'ubuntu20.04-amd64'

test_connection_timeout_tries = 10

# For all retries this is the total amount of time we wait for all operations
retry_wait_time_in_seconds = 60


#
# Other variables that should be available to all tests in this scenario may be added here
#
