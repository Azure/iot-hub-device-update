/**
 * @file device_provisioning_client.h
 * @brief Header for device provisioning client for ADUC.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_DEVICE_PROVISIONING_CLIENT_H
#define ADUC_DEVICE_PROVISIONING_CLIENT_H

#include <aduc/result.h>
#include <azure_c_shared_utility/strings.h>
#include <stdbool.h>

typedef enum tagADUC_ProvisioningAttestation
{
    ADUC_ProvisioningAttestation_Default = 0, /**< The default attestation type. */
    ADUC_ProvisioningAttestation_SymmetricKey, /**< The symmetric key using SAS URL attestation. Suitable only for testing, not production. */
    ADUC_ProvisioningAttestation_X509Certificate, /**< X.509 Certification attestation. Strongly recommended for production devices. */
} ADUC_ProvisioningAttestation;

typedef struct tagADUC_Provisioning_Options
{
    ADUC_ProvisioningAttestation attestationType; /**< The type of device provisioning attestion to use. */
    bool useWebSockets; /**< Whether to use MQTT over websockets or not, port 443 vs 8883, respectively. */
    char* proxy_address; /**< The proxy address for the provisioning. */
    int proxy_port; /**< The proxy port for the provisioning. */
    char* prov_uri; /**< The provisioning server URI. */
    unsigned int max_timeout_seconds; /**< The max timeout in seconds for provisioning the device. */
} ADUC_Provisioning_Options;

ADUC_Result ADUC_DeviceProvisioning_RetrieveConnectionString(const ADUC_Provisioning_Options options, STRING_HANDLE* outConnectionString);

#endif // ADUC_DEVICE_PROVISIONING_CLIENT_H
