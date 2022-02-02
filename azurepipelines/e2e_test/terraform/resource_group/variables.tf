# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
# Local testing - must create a SP and plumb client_id/client_secret below
variable "client_id" {}
variable "client_secret" {}
variable "subscription_id" {}
variable "tenant_id" {}

variable resource_group_name_prefix {
    default = "du-e2etest-rg"
    description = "Concatenated with a random two word petname to form the Resource Group name for the deployment"
}


variable "resource_group_location" {
  default = "eastus"
  description   = "Location of the resource group."
}

variable "environment_tag" {
    default = "du-e2etest"
    description = "Tag used for managing generated resources for infrastructure tear down"
}
