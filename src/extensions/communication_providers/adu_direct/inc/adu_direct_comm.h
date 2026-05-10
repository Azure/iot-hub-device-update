/**
 * @file adu_direct_comm.h
 * @brief ADU Direct Communication Provider - polls ADU REST API for deployments.
 */

#ifndef ADU_DIRECT_COMM_H
#define ADU_DIRECT_COMM_H

#include "aduc/communication_vtable.h"
#include "aduc/extension_context.h"
#include "aduc/extension_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize the ADU Direct communication provider extension.
     * @param ctx Extension context provided by the host.
     * @return ADUC_Result2 indicating success or failure.
     */
    ADUC_Result2 AduDirect_Initialize(const ADUC_ExtensionContext* ctx);

    /**
     * @brief Uninitialize the ADU Direct communication provider extension.
     */
    void AduDirect_Uninitialize(void);

    /**
     * @brief Establish connection to the ADU service endpoint.
     * @param config Connection configuration (endpoint, deviceId, etc.).
     * @return ADUC_Result2 indicating success or failure.
     */
    ADUC_Result2 AduDirect_Connect(const ADUC_CommConfig* config);

    /**
     * @brief Disconnect from the ADU service endpoint and release resources.
     */
    void AduDirect_Disconnect(void);

    /**
     * @brief Query current connection state.
     * @return Current connection state.
     */
    ADUC_CommConnectionState AduDirect_GetConnectionState(void);

    /**
     * @brief Poll the ADU service for available deployments.
     * @param outMsg Output message populated if a deployment is available.
     * @param timeoutMs Timeout in milliseconds for the HTTP request.
     * @return ADUC_Result2 indicating success (message available), no-message, or error.
     */
    ADUC_Result2 AduDirect_Poll(ADUC_CommMessage* outMsg, uint32_t timeoutMs);

    /**
     * @brief Report device state to the ADU service.
     * @param state Agent state struct.
     * @return ADUC_Result2 indicating success or failure.
     */
    ADUC_Result2 AduDirect_ReportState(const ADUC_AgentState* state);

    /**
     * @brief Report deployment result to the ADU service.
     * @param result Deployment result struct.
     * @return ADUC_Result2 indicating success or failure.
     */
    ADUC_Result2 AduDirect_ReportResult(const ADUC_DeploymentResult2* result);

    /**
     * @brief Get a download URL for a specific file.
     * @param fileId The file identifier.
     * @param urlBuf Buffer to receive the download URL.
     * @param urlBufLen Size of the output buffer.
     * @return ADUC_Result2 indicating success or failure.
     */
    ADUC_Result2 AduDirect_GetDownloadUrl(const char* fileId, char* urlBuf, size_t urlBufLen);

    /**
     * @brief Perform a health check against the ADU service.
     * @param outStatus Output health status.
     * @return ADUC_Result2 indicating success or failure.
     */
    ADUC_Result2 AduDirect_HealthCheck(ADUC_CommHealthStatus* outStatus);

    /**
     * @brief Register a callback for communication events (stub).
     * @param event Event type to register for.
     * @param cb The callback function pointer.
     * @param ctx User-provided context passed to the callback.
     * @return ADUC_Result2 indicating success.
     */
    ADUC_Result2 AduDirect_RegisterCallback(ADUC_CommEventType event, ADUC_CommCallback cb, void* ctx);

    /**
     * @brief Send diagnostics data to the ADU service (stub).
     * @param payload Diagnostics payload.
     * @param payloadLen Length of the payload.
     * @return ADUC_Result2 indicating success.
     */
    ADUC_Result2 AduDirect_SendDiagnostics(const void* payload, size_t payloadLen);

#ifdef __cplusplus
}
#endif

#endif /* ADU_DIRECT_COMM_H */
