# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
# Local testing - must create a SP and plumb client_id/client_secret below using scripts/create-sp-terraform.sh
variable "client_id" {}
variable "client_secret" {}
variable "subscription_id" {}
variable "tenant_id" {}
variable "key_vault_id" {}
variable "resource_group_name" {}

# Tag used for managing generated resources in infrastructure tear down
variable "environment_tag" {
    default = "du-e2etest"
}

variable "vm_name" {
    default = "autest-device"
}

variable "vm_size" {
    default = "Standard_DS1_v2"
}

variable "image_publisher" {
    default = "Canonical"
}

variable "image_offer" {
    default = "UbuntuServer"
}

variable "image_sku" {
    default = "18.04-LTS"
}

variable "image_version" {
    default = "latest"
}

variable "test_setup_tarball"{
    default = ""
}

variable "vm_du_tarball_script" {
    default = ""
}

# Other variables should be put here
