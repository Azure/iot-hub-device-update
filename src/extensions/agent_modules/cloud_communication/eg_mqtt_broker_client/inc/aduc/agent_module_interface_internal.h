#ifndef AGENT_MODULE_INTERFACE_INTERNAL_H
#define AGENT_MODULE_INTERFACE_INTERNAL_H

#include <du_agent_sdk/agent_module_interface.h> // ADUC_AGENT_CONTRACT_INFO

//
// The per operation module interface declaration expansions
//

// Enrollment request and response management

const ADUC_AGENT_CONTRACT_INFO* ADUC_Enrollment_Management_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle);
int ADUC_Enrollment_Management_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* initData);
int ADUC_Enrollment_Management_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle);
int ADUC_Enrollment_Management_DoWork(ADUC_AGENT_MODULE_HANDLE handle);
void ADUC_Enrollment_Management_Destroy(ADUC_AGENT_MODULE_HANDLE handle);

// AgentInfo request and response management

const ADUC_AGENT_CONTRACT_INFO* ADUC_AgentInfo_Management_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle);
int ADUC_AgentInfo_Management_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* initData);
int ADUC_AgentInfo_Management_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle);
int ADUC_AgentInfo_Management_DoWork(ADUC_AGENT_MODULE_HANDLE handle);
void ADUC_AgentInfo_Management_Destroy(ADUC_AGENT_MODULE_HANDLE handle);

// Update request and response management

const ADUC_AGENT_CONTRACT_INFO* ADUC_Update_Management_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle);
int ADUC_Update_Management_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* initData);
int ADUC_Update_Management_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle);
int ADUC_Update_Management_DoWork(ADUC_AGENT_MODULE_HANDLE handle);
void ADUC_Update_Management_Destroy(ADUC_AGENT_MODULE_HANDLE handle);

// UpdateResults request and response management

// const ADUC_AGENT_CONTRACT_INFO* ADUC_UpdateResults_Management_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle);
// int ADUC_UpdateResults_Management_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* initData);
// int ADUC_UpdateResults_Management_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle);
// int ADUC_UpdateResults_Management_DoWork(ADUC_AGENT_MODULE_HANDLE handle);
// void ADUC_UpdateResults_Management_Destroy(ADUC_AGENT_MODULE_HANDLE handle);

#endif // AGENT_MODULE_INTERFACE_INTERNAL_H
