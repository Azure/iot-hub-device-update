# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
output "du_resource_group_name" {
    value = azurerm_resource_group.du-resource-group.name
}
