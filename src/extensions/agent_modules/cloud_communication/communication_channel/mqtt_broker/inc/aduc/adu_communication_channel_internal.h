/**
 * @file adu_communication_channel_internal.h
 * @brief Header file for the Device Update communication channel (MQTT broker) management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __ADUC_COMMUNICATION_CHANNEL_INTERNAL_H__
#define __ADUC_COMMUNICATION_CHANNEL_INTERNAL_H__

#include "aduc/adu_communication_channel.h"

EXTERN_C_BEGIN

/*
 * @brief  Initialize the communication channel.
 */
int ADUC_Communication_Channel_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void *moduleInitData);


/*
 * @brief Deinitialize the communication channel.
 */
int ADUC_Communication_Channel_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle);

/**
 * @brief Ensure that the communication channel to the DU service is valid.
 *
 * @remark This function requires that the MQTT client is initialized.
 * @remark This function requires that the MQTT connection settings are valid.
 *
 * @return true if the communication channel state is ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED.
 */
bool ADUC_CommunicationChannel_DoWork();

EXTERN_C_END

#endif // __ADUC_COMMUNICATION_CHANNEL_INTERNAL_H__
