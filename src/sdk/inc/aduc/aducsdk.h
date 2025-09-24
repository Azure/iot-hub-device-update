#ifndef ADUC_SDK_H
#define ADUC_SDK_H

typedef enum tagADUC_ServiceStatus {
   // V1.0 states
   ADUC_ServiceStatus_None         = 0,
   ADUC_ServiceStatus_Initializing = 1,
   ADUC_ServiceStatus_Downloading  = 2,
   ADUC_ServiceStatus_Installing   = 3,
   ADUC_ServiceStatus_Rebooting    = 4,
   ADUC_ServiceStatus_Reporting    = 5,
   ADUC_ServiceStatus_Paused       = 6,
   ADUC_ServiceStatus_Idle         = 7,

   // V1.0 Error codes (10000+)
   ADUC_ServiceStatus_ERROR_UnsupportedApiVersion  = 10000,
   ADUC_ServiceStatus_ERROR_AgentServiceNotRunning = 10001,
   ADUC_ServiceStatus_ERROR_AgentServiceBrokenPipe = 10002,
   ADUC_ServiceStatus_ERROR_AgentServicePermission = 10003,
   ADUC_ServiceStatus_ERROR_AgentServiceTimeout    = 10004,
   ADUC_ServiceStatus_ERROR_OutOfMemory            = 10005,
   ADUC_ServiceStatus_ERROR_Unknown                = 99999,
} ADUC_ServiceStatus;

#endif // ADUC_SDK_H
