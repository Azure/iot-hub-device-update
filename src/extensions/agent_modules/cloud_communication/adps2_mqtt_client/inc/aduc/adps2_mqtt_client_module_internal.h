#ifndef __ADPS2_MQTT_CLIENT_MODULE_INTERNAL_H__
#define __ADPS2_MQTT_CLIENT_MODULE_INTERNAL_H__

/**
 * @file adps2-mqtt-client-module-internal.h
 * @brief Implementation for the private APIs for the Azure Device Provisioning MQTT client module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adps2_mqtt_client_module.h"



/**
 * @brief Gets the extension contract info.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @return ADUC_AGENT_CONTRACT_INFO The extension contract info.
 */
const ADUC_AGENT_CONTRACT_INFO* ADPS_MQTT_Client_Module_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle);

/*
 * Callback called when the client receives a CONNACK message from the broker.
 */
void ADPS_OnConnect(struct mosquitto* mosq, void* obj, int reason_code, int flags, const mosquitto_property* props);

/*
 * Callback called when the broker has received the DISCONNECT command and has disconnected the
 * client.
 */
void ADPS_OnDisconnect(struct mosquitto* mosq, void* obj, int rc, const mosquitto_property* props);

/* Callback called when the client receives a message. */
void ADPS_OnMessage(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void ADPS_OnPublish(struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props);

/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void ADPS_OnSubscribe(
    struct mosquitto* mosq,
    void* obj,
    int mid,
    int qos_count,
    const int* granted_qos,
    const mosquitto_property* props);

void ADPS_OnLog(struct mosquitto* mosq, void* obj, int level, const char* str);

/**
 * @brief Initialize the IoTHub Client module.
 */
int ADPS_MQTT_Client_Module_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData);

/**
 * @brief Deinitialize the IoTHub Client module.
 * @param module The handle to the module. This is the same handle that was returned by the Create function.
 * @return int 0 on success.
 */
int ADPS_MQTT_Client_Module_Deinitialize(ADUC_AGENT_MODULE_HANDLE module);


/**
 * @brief Perform the work for the extension. This must be a non-blocking operation.
 *
 * @return ADUC_Result
 */
int ADPS_MQTT_Client_Module_DoWork(ADUC_AGENT_MODULE_HANDLE handle);

#endif // __ADPS2_MQTT_CLIENT_MODULE_H__
