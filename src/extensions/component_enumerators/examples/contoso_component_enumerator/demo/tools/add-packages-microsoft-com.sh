#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

curl https://packages.microsoft.com/config/ubuntu/22.04/packages-microsoft-prod.deb > ~/packages-microsoft-prod.deb

sudo apt-get install ~/packages-microsoft-prod.deb

sudo apt-get update
