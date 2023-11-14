#ifndef TOPIC_MGMT_MODULE_LIFECYCLE_H
#define TOPIC_MGMT_MODULE_LIFECYCLE_H

#include <aduc/c_utils.h> // EXTERN_C_BEGIN, EXTERN_C_END
#include <du_agent_sdk/agent_module_interface.h> // ADUC_AGENT_MODULE_HANDLE

EXTERN_C_BEGIN

/**
 * @brief The supported well-known MQTT topic management modules.
 * @details The order of these enums must stay in-sync with LifeCycleFn array in topic_mgmt_lifecycle.c
 */
typedef enum tagTOPIC_MGMT_MODULE
{
    TOPIC_MGMT_MODULE_Enrollment = 0, ///< Enrollment management module topic
    TOPIC_MGMT_MODULE_AgentInfo, ///< AgentInfo management module topic
    TOPIC_MGMT_MODULE_Update, ///< Update management module topic
    TOPIC_MGMT_MODULE_UpdateResults, ///< UpdateResults management module topic
} TOPIC_MGMT_MODULE;

//
// Public interface
//

ADUC_AGENT_MODULE_HANDLE TopicMgmtLifecycle_Create(TOPIC_MGMT_MODULE modTopic);
void TopicMgmtLifecycle_Destroy(ADUC_AGENT_MODULE_HANDLE handle);

EXTERN_C_END

#endif // TOPIC_MGMT_MODULE_LIFECYCLE_H
