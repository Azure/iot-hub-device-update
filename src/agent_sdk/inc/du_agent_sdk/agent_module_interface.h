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

EXTERN_C_BEGIN

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
    ADUC_ModuleDataType_Int64 = 3, /**< 64-bit integer data */
    ADUC_ModuleDataType_Boolean = 4, /**< Boolean data */
    ADUC_ModuleDataType_Binary = 5, /**< Binary data */
} ADUC_MODULE_DATA_TYPE;

typedef const ADUC_AGENT_CONTRACT_INFO* (*ADUC_AGENT_MODULE_GET_CONTRACT_INFO)(ADUC_AGENT_MODULE_HANDLE);
typedef int (*ADUC_AGENT_MODULE_DO_WORK)(ADUC_AGENT_MODULE_HANDLE);
typedef int (*ADUC_AGENT_MODULE_INITIALIZE)(ADUC_AGENT_MODULE_HANDLE, void*);
typedef int (*ADUC_AGENT_MODULE_DEINITIALIZE)(ADUC_AGENT_MODULE_HANDLE);
typedef int (*ADUC_AGENT_MODULE_GET_DATA)(ADUC_AGENT_MODULE_HANDLE, ADUC_MODULE_DATA_TYPE, int, void*, int*);
typedef int (*ADUC_AGENT_MODULE_SET_DATA)(ADUC_AGENT_MODULE_HANDLE, ADUC_MODULE_DATA_TYPE, int, void*, int);

/**
 * @brief The Device Update agent module interface contains the function pointers for
 * all the functions that a module must implement.
 *
 */
typedef struct ADUC_AGENT_MODULE_INTERFACE_TAG
{
    void* moduleData;
    ADUC_AGENT_MODULE_CREATE create;
    ADUC_AGENT_MODULE_DESTROY destroy;
    ADUC_AGENT_MODULE_GET_CONTRACT_INFO getContractInfo;
    ADUC_AGENT_MODULE_DO_WORK doWork;
    ADUC_AGENT_MODULE_INITIALIZE initializeModule;
    ADUC_AGENT_MODULE_DEINITIALIZE deinitializeModule;
    ADUC_AGENT_MODULE_GET_DATA getData;
    ADUC_AGENT_MODULE_SET_DATA setData;
} ADUC_AGENT_MODULE_INTERFACE;

#define DECLARE_AGENT_MODULE_PUBLIC(name)     \
    ADUC_AGENT_MODULE_HANDLE name##_Create(); \
    void name##_Destroy(ADUC_AGENT_MODULE_HANDLE moduleHandle);

#define DECLARE_AGENT_MODULE_PRIVATE(name)                                                                      \
    int name##_Initialize(ADUC_AGENT_MODULE_HANDLE moduleHandle, void* initData);                               \
    void name##_Deinitialize(ADUC_AGENT_MODULE_HANDLE moduleHandle);                                            \
    int name##_DoWork(ADUC_AGENT_MODULE_HANDLE moduleHandle);                                                   \
    int name##_GetData(                                                                                         \
        ADUC_AGENT_MODULE_HANDLE moduleHandle, ADUC_MODULE_DATA_TYPE dataType, int key, void* data, int* size); \
    int name##_SetData(                                                                                         \
        ADUC_AGENT_MODULE_HANDLE moduleHandle, ADUC_MODULE_DATA_TYPE dataType, int key, void* data, int size);

EXTERN_C_END

#endif // __ADUC_AGENT_MODULE_INTERFACE_H__
