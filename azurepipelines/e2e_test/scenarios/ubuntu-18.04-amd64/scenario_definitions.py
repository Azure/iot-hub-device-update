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

sys.path.append('./')
from testingtoolkit import UpdateId

#
# Base Device Definitions
#
test_device_id = "ubuntu-18.04-apt-deployment-test-device"
test_adu_group = "Ubuntu1804AduGroup"
test_apt_deployment_id = "ubuntu1804aptdeploymenttest"
test_result_file_prefix = 'ubuntu-18.04-amd64'
#
# Other variables that should be available to all tests in this scenario may be added here
#
