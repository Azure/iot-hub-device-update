#ifndef ADUC_SDK_H
#define ADUC_SDK_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum tagADUC_ServiceStatus
    {
        // V1.0 states
        ADUC_ServiceStatus_None = 0,
        ADUC_ServiceStatus_Initializing = 1,
        ADUC_ServiceStatus_Downloading = 2,
        ADUC_ServiceStatus_Installing = 3,
        ADUC_ServiceStatus_Applying = 4,
        ADUC_ServiceStatus_Cancelling = 5,
        ADUC_ServiceStatus_Reporting = 6,
        ADUC_ServiceStatus_Rebooting = 7,
        ADUC_ServiceStatus_Paused = 8,
        ADUC_ServiceStatus_Idle = 9,
        ADUC_ServiceStatus_Failed = 10,

        // V1.0 Error codes (10000+)
        ADUC_ServiceStatus_ERROR_UnsupportedApiVersion = 10000,
        ADUC_ServiceStatus_ERROR_AgentServiceNotRunning = 10001,
        ADUC_ServiceStatus_ERROR_AgentServiceBrokenPipe = 10002,
        ADUC_ServiceStatus_ERROR_AgentServicePermission = 10003,
        ADUC_ServiceStatus_ERROR_AgentServiceTimeout = 10004,
        ADUC_ServiceStatus_ERROR_AgentServiceInternal = 10005,
        ADUC_ServiceStatus_ERROR_AgentServiceReqFifoSvcEndNotOpenedYet = 10006,
        ADUC_ServiceStatus_ERROR_AgentServiceMkFifoFailed = 10007,
        ADUC_ServiceStatus_ERROR_AgentServiceChmodFailed = 10008,
        ADUC_ServiceStatus_ERROR_AgentServiceSdkOpenRespFifoFailed = 10009,
        ADUC_ServiceStatus_ERROR_RecvMsgFailed = 10010,
        ADUC_ServiceStatus_ERROR_Unknown = 99999,
    } ADUC_ServiceStatus;

    /**
     * @brief Gets the ADU IoT Agent service daemon's current status
     * @return Current service status or error code
     */
    ADUC_ServiceStatus GetAduServiceStatus(void);

    /**
     * @brief Gets human-readable string for status code
     * @param status The status code
     * @return The string representation (do not free the string)
     */
    const char* ADUC_ServiceStatusToString(ADUC_ServiceStatus status);

#ifdef __cplusplus
}
#endif

#endif // ADUC_SDK_H
