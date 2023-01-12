# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
# Configure the Microsoft Azure Provider
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

  # Must create a SP and plumb id/secret below using scripts/create-sp-terraform.sh
  # if manually running from a machine these must be generated and plumbed through
  client_id         = var.client_id
  client_secret     = var.client_secret
}

# Create public IPs
resource "azurerm_public_ip" "deviceupdatepublicip" {
    name                         = "myPublicIP-${var.vm_name}"
    resource_group_name          = var.resource_group_name
    location                     = "eastus"
    allocation_method            = "Dynamic"

    tags = {
        environment = var.environment_tag
    }
}

# Create virtual network
resource "azurerm_virtual_network" "deviceupdatenetwork" {
    name                = "myVnet-${var.vm_name}"
    address_space       = ["10.0.0.0/16"]
    resource_group_name = var.resource_group_name
    location            = "eastus"

    tags = {
        environment = var.environment_tag
    }
}

# Create subnet
resource "azurerm_subnet" "deviceupdatesubnet" {
    name                 = "mySubnet-${var.vm_name}"
    resource_group_name  = var.resource_group_name
    virtual_network_name = azurerm_virtual_network.deviceupdatenetwork.name
    address_prefixes       = ["10.0.1.0/24"]
}

# Create Network Security Group and rule
resource "azurerm_network_security_group" "deviceupdatensg" {
    name                = "myNetworkSecurityGroup-${var.vm_name}"
    location            = "eastus"
    resource_group_name = var.resource_group_name


    security_rule {
        name                       = "SSH"
        priority                   = 1001
        direction                  = "Inbound"
        access                     = "Allow"
        protocol                   = "Tcp"
        source_port_range          = "*"
        destination_port_range     = "22"
        source_address_prefix      = "*"
        destination_address_prefix = "*"
    }

    tags = {
        environment = var.environment_tag
    }
}

# Create network interface
resource "azurerm_network_interface" "deviceupdatenic" {
    name                      = "myNIC-${var.vm_name}"
    location                  = "eastus"
    resource_group_name       = var.resource_group_name

    ip_configuration {
        name                          = "myNicConfiguration-${var.vm_name}"
        subnet_id                     = azurerm_subnet.deviceupdatesubnet.id
        private_ip_address_allocation = "Dynamic"
        public_ip_address_id          = azurerm_public_ip.deviceupdatepublicip.id
    }

    tags = {
        environment = var.environment_tag
    }
}

# Connect the security group to the network interface
resource "azurerm_network_interface_security_group_association" "example" {
    network_interface_id      = azurerm_network_interface.deviceupdatenic.id
    network_security_group_id = azurerm_network_security_group.deviceupdatensg.id
}

# Create an SSH key - export to PKCS#8 (Dependency: OpenSSL)
resource "tls_private_key" "deviceupdate_ssh_key" {
  algorithm = "RSA"
  rsa_bits = 4096
}

# Add secret into keyvault
resource "azurerm_key_vault_secret" "vm_ssh_secret" {
  name         = "${var.resource_group_name}-${replace(var.vm_name, ".", "")}"
  value        = tls_private_key.deviceupdate_ssh_key.private_key_pem
  key_vault_id = var.key_vault_id
  # expire in 30 days
  expiration_date = timeadd(timestamp(), "720h")
}
# Create virtual machine
resource "azurerm_linux_virtual_machine" "deviceupdatevm" {
    name                  = "myVM-${var.vm_name}"
    location              = "eastus"
    resource_group_name   = var.resource_group_name
    network_interface_ids = [azurerm_network_interface.deviceupdatenic.id]
    size                  = var.vm_size

    os_disk {
        name              = "myOsDisk-${var.vm_name}"
        caching           = "ReadWrite"
        storage_account_type = "Premium_LRS"
    }

    source_image_reference {
        publisher = var.image_publisher
        offer     = var.image_offer
        sku       = var.image_sku
        version   = var.image_version
    }

    computer_name  = "myvm-${var.vm_name}"
    admin_username = "azureuser"
    disable_password_authentication = true

    admin_ssh_key {
        username       = "azureuser"
        public_key     = tls_private_key.deviceupdate_ssh_key.public_key_openssh
    }

    connection {
        host        = self.public_ip_address
        type        = "ssh"
        port        = 22
        user        = "azureuser"
        private_key = tls_private_key.deviceupdate_ssh_key.private_key_pem
        timeout     = "1m"
    }

    # Use remote-exec to install + run + foregroud to not block terraform job
    # expects there to be a tarball for the du setup at /tmp/testsetup.tar.gz
    provisioner "file" {
        source      = var.test_setup_tarball
        destination = "/tmp/testsetup.tar.gz"
    }

    #
    # Changes to provide the setup information should go here
    #
    provisioner "remote-exec" {
        inline = [
        "sudo apt update",
        var.vm_du_tarball_script
        ]
    }

    tags = {
        environment = var.environment_tag
    }
}

resource "azurerm_dev_test_global_vm_shutdown_schedule" "vm_shutdown_schedule" {
  virtual_machine_id = azurerm_linux_virtual_machine.deviceupdatevm.id
  location           = "eastus"
  enabled            = true

  // Shutdown 60-mins after creation time for cost savings
  daily_recurrence_time = formatdate("hhmm", timeadd(timestamp(), "180m"))
  timezone              = "UTC"

  notification_settings {
      enabled = false
  }
}
