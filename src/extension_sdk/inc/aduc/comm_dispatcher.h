/**
 * @file comm_dispatcher.h
 * @brief Communication dispatcher — routes operations through prioritized communication providers.
 *
 * The dispatcher manages multiple communication providers (e.g., ADU Direct, IoT Hub),
 * routing poll operations through them in priority order with failover, and broadcasting
 * reporting operations to all connected providers.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_COMM_DISPATCHER_H
#define ADUC_COMM_DISPATCHER_H

#include "aduc/communication_vtable.h"
#include "aduc/extension_loader.h"
#include "aduc/extension_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Opaque dispatcher handle.
 */
typedef struct ADUC_CommDispatcher* ADUC_CommDispatcherHandle;

/**
 * @brief Configuration for a single communication provider entry.
 */
typedef struct ADUC_CommProviderConfig
{
    const char* extensionId; /**< Extension ID to load from registry. */
    uint32_t priority;       /**< Lower value = higher priority. */
    bool reportOnly;         /**< If true, only used for reporting (not polling). */
} ADUC_CommProviderConfig;

/**
 * @brief Dispatcher configuration.
 */
typedef struct ADUC_CommDispatcherConfig
{
    ADUC_CommProviderConfig* providers; /**< Array of provider configurations. */
    size_t providerCount;              /**< Number of entries in providers array. */
    uint32_t failoverTimeoutMs;        /**< Time to wait before trying next provider on failure. */
} ADUC_CommDispatcherConfig;

/**
 * @brief Create a communication dispatcher.
 *
 * Looks up each configured extension from the registry and obtains vtable pointers.
 *
 * @param config     Dispatcher configuration (providers, priorities).
 * @param registry   Extension registry to look up provider extensions.
 * @param outHandle  Receives the created dispatcher handle.
 * @return ADUC_Result2 Success or error code.
 */
ADUC_Result2 ADUC_CommDispatcher_Create(
    const ADUC_CommDispatcherConfig* config,
    ADUC_ExtensionRegistryHandle registry,
    ADUC_CommDispatcherHandle* outHandle);

/**
 * @brief Connect all configured providers.
 *
 * Calls Connect on each provider's vtable. Providers that fail to connect are
 * marked as disconnected but do not prevent other providers from connecting.
 *
 * @param handle     Dispatcher handle.
 * @param commConfig Communication configuration passed to each provider.
 * @return ADUC_Result2 Success if at least one provider connected, error if none did.
 */
ADUC_Result2 ADUC_CommDispatcher_ConnectAll(
    ADUC_CommDispatcherHandle handle,
    const ADUC_CommConfig* commConfig);

/**
 * @brief Poll for deployments.
 *
 * Tries providers in priority order (skipping reportOnly providers).
 * Returns the first successful result. On failure, fails over to the next provider.
 *
 * @param handle    Dispatcher handle.
 * @param outMsg    Output message if a deployment is available.
 * @param timeoutMs Maximum time to wait per provider.
 * @return ADUC_Result2 Success if a message was received, error if all providers failed.
 */
ADUC_Result2 ADUC_CommDispatcher_Poll(
    ADUC_CommDispatcherHandle handle,
    ADUC_CommMessage* outMsg,
    uint32_t timeoutMs);

/**
 * @brief Report agent state to all connected providers.
 *
 * Broadcasts the state report to every connected provider.
 *
 * @param handle Dispatcher handle.
 * @param state  Agent state to report.
 * @return ADUC_Result2 Success if at least one provider succeeded, error if all failed.
 */
ADUC_Result2 ADUC_CommDispatcher_ReportState(
    ADUC_CommDispatcherHandle handle,
    const ADUC_AgentState* state);

/**
 * @brief Report deployment result to all connected providers.
 *
 * Broadcasts the result to every connected provider.
 *
 * @param handle Dispatcher handle.
 * @param result Deployment result to report.
 * @return ADUC_Result2 Success if at least one provider succeeded, error if all failed.
 */
ADUC_Result2 ADUC_CommDispatcher_ReportResult(
    ADUC_CommDispatcherHandle handle,
    const ADUC_DeploymentResult2* result);

/**
 * @brief Get overall health status across all providers.
 *
 * Returns CONNECTED if all providers are healthy, DEGRADED if some are,
 * DISCONNECTED if none are connected.
 *
 * @param handle    Dispatcher handle.
 * @param outStatus Receives the aggregate health status.
 * @return ADUC_Result2 Success or error.
 */
ADUC_Result2 ADUC_CommDispatcher_HealthCheck(
    ADUC_CommDispatcherHandle handle,
    ADUC_CommHealthStatus* outStatus);

/**
 * @brief Disconnect all providers and destroy the dispatcher.
 *
 * @param handle Dispatcher handle (may be NULL).
 */
void ADUC_CommDispatcher_Destroy(ADUC_CommDispatcherHandle handle);

#ifdef __cplusplus
}
#endif

#endif /* ADUC_COMM_DISPATCHER_H */
