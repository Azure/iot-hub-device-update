/**
 * @file agent_module_interface.h
 * @brief Defines types and methods for DU Agent module library.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __ADUC_AGENT_MODULE_INTERFACE_H__
#define __ADUC_AGENT_MODULE_INTERFACE_H__

#include "aduc/result.h"
#include "du_agent_sdk/agent_common.h"

#ifdef __cplusplus
extern "C"
{
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
        ADUC_ModuleDataType_None = 0, /**< No module data*/
        ADUC_ModuleDataType_String = 1, /**< String data */
        ADUC_ModuleDataType_Json = 2, /**< JSON string data */
        ADUC_ModuleDataType_Binary = 3, /**< Binary data */
    } ADUC_MODULE_DATA_TYPE;

    typedef const ADUC_AGENT_CONTRACT_INFO* (*ADUC_AGENT_MODULE_GET_CONTRACT_INFO)(ADUC_AGENT_MODULE_HANDLE);
    typedef int (*ADUC_AGENT_MODULE_DO_WORK)(ADUC_AGENT_MODULE_HANDLE);
    typedef int (*ADUC_AGENT_MODULE_INITIALIZE)(ADUC_AGENT_MODULE_HANDLE, void*);
    typedef int (*ADUC_AGENT_MODULE_DEINITIALIZE)(ADUC_AGENT_MODULE_HANDLE);
    typedef int (*ADUC_AGENT_MODULE_GET_DATA)(
        ADUC_AGENT_MODULE_HANDLE, ADUC_MODULE_DATA_TYPE, const char*, void**, int*);

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // #ifndef #define __ADUC_AGENT_MODULE_INTERFACE_H__