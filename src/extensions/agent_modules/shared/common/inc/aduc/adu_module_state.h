
#ifndef ADU_MODULE_STATE_H
#define ADU_MODULE_STATE_H

#include <aducpal/time.h> // time_t
#include <du_agent_sdk/agent_module_interface.h> // ADUC_AGENT_MODULE_INTERFACE
#include <du_agent_sdk/mqtt_client_settings.h> // for ADUC_MQTT_SETTINGS


/**
 * @brief The module state.
 */
typedef struct tagADUC_MQTT_CLIENT_MODULE_STATE
{
    void* moduleInitData; //!< Module init data
    ADUC_MQTT_SETTINGS mqttSettings; //!< MQTT connection settings

    char* mqtt_topic_service2agent; // For v1, we only subscribe to 1 topic.
    char* mqtt_topic_agent2service; // For v1, we only publish to 1 topic.

    time_t lastErrorTime; //!< Last time an error occurred
    int lastError; //!< Last error code
    time_t nextOperationTime; //!< Next time to perform an operation

    ADUC_AGENT_MODULE_INTERFACE* commChannelModule; //!< The communication module handle
    ADUC_AGENT_MODULE_INTERFACE* enrollmentModule; //!< The enrollment module handle
    ADUC_AGENT_MODULE_INTERFACE* agentInfoModule; //!< The agent info module handle
} ADUC_MQTT_CLIENT_MODULE_STATE;

#endif // ADU_MODULE_STATE_H
