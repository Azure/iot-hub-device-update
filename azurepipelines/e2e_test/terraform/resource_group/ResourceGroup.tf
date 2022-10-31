# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
terraform {
  required_providers {
    azurerm = {
      source = "hashicorp/azurerm"
      version = "~>2.0"
    }
  }
}
provider "azurerm" {
  features {}

  subscription_id   = var.subscription_id
  tenant_id         = var.tenant_id

  # Local testing - must create a SP and plumb id/secret below using scripts/create-sp-terraform.sh
  client_id         = var.client_id
  client_secret     = var.client_secret
}

resource "random_pet" "rg-name" {
  prefix    = var.resource_group_name_prefix
}

resource "azurerm_resource_group" "du-resource-group" {
  name      = random_pet.rg-name.id
  location  = var.resource_group_location

  tags = {
      environment = var.environment_tag
  }
}
