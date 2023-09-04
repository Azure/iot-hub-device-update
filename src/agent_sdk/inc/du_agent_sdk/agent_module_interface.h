/**
 * @file agent_module_interface.h
 * @brief Defines types and methods for DU Agent module library.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_AGENT_MODULE_INTERFACE_H
#define ADUC_AGENT_MODULE_INTERFACE_H

#include "du_agent_sdk/agent_common.h"
#include "aduc/result.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void* ADUC_AGENT_MODULE_HANDLE;

typedef ADUC_AGENT_MODULE_HANDLE (*ADUC_AGENT_MODULE_CREATE)(void);
typedef void (*ADUC_AGENT_MODULE_DESTROY)(ADUC_AGENT_MODULE_HANDLE);

typedef struct ADUC_AGENT_CONTRACT_INFO_TAG
{
    const char* provider;
    const char* name;
    const int version;
    const char* contractInfo;
} ADUC_AGENT_CONTRACT_INFO;

/**
 * @brief The module data types
 *
 */
typedef enum ADUC_MODULE_DATA_TYPE_TAG
{
    ADUC_ModuleDataType_None = 0,   /**< No module data*/
    ADUC_ModuleDataType_String = 1, /**< String data */
    ADUC_ModuleDataType_Json = 2,   /**< JSON string data */
    ADUC_ModuleDataType_Binary = 3, /**< Binary data */
} ADUC_MODULE_DATA_TYPE;


typedef const ADUC_AGENT_CONTRACT_INFO* (*ADUC_AGENT_MODULE_GET_CONTRACT_INFO)(ADUC_AGENT_MODULE_HANDLE);
typedef int (*ADUC_AGENT_MODULE_DO_WORK)(ADUC_AGENT_MODULE_HANDLE);
typedef int (*ADUC_AGENT_MODULE_INITIALIZE)(ADUC_AGENT_MODULE_HANDLE, void*);
typedef int (*ADUC_AGENT_MODULE_DEINITIALIZE)(ADUC_AGENT_MODULE_HANDLE);
typedef int (*ADUC_AGENT_MODULE_GET_DATA)(ADUC_AGENT_MODULE_HANDLE, ADUC_MODULE_DATA_TYPE, const char*, void**, int*);

/**
 * @brief The Device Update agent module interface contains the function pointers for
 * all the functions that a module must implement.
 *
 */
typedef struct ADUC_AGENT_MODULE_INTERFACE_TAG
{
    ADUC_AGENT_MODULE_HANDLE moduleHandle;
    ADUC_AGENT_MODULE_CREATE create;
    ADUC_AGENT_MODULE_DESTROY destroy;
    ADUC_AGENT_MODULE_GET_CONTRACT_INFO getContractInfo;
    ADUC_AGENT_MODULE_DO_WORK doWork;
    ADUC_AGENT_MODULE_INITIALIZE initializeModule;
    ADUC_AGENT_MODULE_DEINITIALIZE deinitializeModule;
    ADUC_AGENT_MODULE_GET_DATA getData;
} ADUC_AGENT_MODULE_INTERFACE;


// /**
//  * @brief Gets the extension contract info.
//  *
//  * @param[out] contractInfo The extension contract info.
//  * @return ADUC_Result The result.
//  */
// ADUC_Result GetContractInfo(char** contractInfo);

// /**
//  * @brief Create a Device Update Agent Module object
//  *
//  * @param deviceUpdateAgentModule A pointer to the Device Update Agent Module object.
//  * @return ADUC_Result
//  */
// typedef ADUC_Result (*CreateDeviceUpdateAgentWorkflowModuleFunc)(ADUC_DeviceUpdateAgentModule** deviceUpdateAgentModule);

// /**
//  * @brief Perform the work for the extension. This must be a non-blocking operation.
//  *
//  * @return ADUC_Result
//  */
// ADUC_Result DoWork(ADUC_DeviceUpdateAgentModule* module);

// /**
//  * @brief Initialize the module. This is called once when the module is loaded.
//  *
//  * @param module The module to initialize.
//  * @param moduleInitData The module initialization data. Consult the documentation for the specific module for details.
//  * @return ADUC_Result
//  */
// ADUC_Result InitializeModule(ADUC_DeviceUpdateAgentModule* module, void *moduleInitData);

// /**
//  * @brief
//  *
//  * @param module
//  *
//  * @return ADUC_Result
//  */
// ADUC_Result DeinitializeModule(ADUC_DeviceUpdateAgentModule* module);


// /**
//  * @brief Get the Data object for the specified key.
//  *
//  * @param moduleId  name of the module
//  * @param dataType data type
//  * @param key data key/name
//  * @param value return value (call must free the memory of the return value once done with it)
//  * @param size return size of the return value
//  * @return ADUC_Result
//  */
// ADUC_Result ModuleManager_GetData(const char* moduleId, ADUC_ModuleDataType dataType, const char* key, void** value, int* size);
// ADUC_Result ModuleManager_InitializeAgentWorkflowModule(const char* moduleId, int moduleVersion, const char* initializeData, ADUC_DeviceUpdateAgentModule **module);
// ADUC_Result ModuleManager_DeinitializeAgentWorkflowModule(const ADUC_DeviceUpdateAgentModule *module);
//

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // #ifndef ADUC_AGENT_MODULE_INTERFACE_H


