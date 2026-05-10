/**
 * @file dag_engine.h
 * @brief Directed Acyclic Graph scheduler for deployment steps.
 *
 * Steps declare dependencies via dependsOn fields. The engine resolves
 * execution order and supports running independent steps in parallel.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_DAG_ENGINE_H
#define ADUC_DAG_ENGINE_H

#include "aduc/extension_types.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Node execution states.
 */
typedef enum ADUC_DagNodeState {
    DAG_NODE_PENDING = 0,
    DAG_NODE_RUNNING,
    DAG_NODE_DONE,
    DAG_NODE_FAILED,
    DAG_NODE_SKIPPED,
} ADUC_DagNodeState;

typedef struct ADUC_DagNode ADUC_DagNode;
typedef struct ADUC_DagEngine* ADUC_DagEngineHandle;

typedef struct ADUC_DagNodeDef {
    const char* stepId;
    const char** dependsOn;      /**< NULL-terminated array of step IDs */
    size_t dependsOnCount;
    const char** skipOnFailed;   /**< NULL-terminated array of step IDs — skip this if any failed */
    size_t skipOnFailedCount;
    const char** runOnFailed;    /**< NULL-terminated array of step IDs — only run if any failed */
    size_t runOnFailedCount;
} ADUC_DagNodeDef;

/**
 * @brief Create DAG from step definitions.
 */
ADUC_Result2 ADUC_DagEngine_Create(const ADUC_DagNodeDef* nodes, size_t nodeCount, ADUC_DagEngineHandle* outHandle);

/**
 * @brief Get next batch of ready nodes (no unmet dependencies).
 * @return Number of step IDs written to outStepIds.
 */
size_t ADUC_DagEngine_GetReady(ADUC_DagEngineHandle handle, const char** outStepIds, size_t maxCount);

/**
 * @brief Mark a node as completed.
 */
void ADUC_DagEngine_MarkDone(ADUC_DagEngineHandle handle, const char* stepId);

/**
 * @brief Mark a node as failed (blocks dependents).
 */
void ADUC_DagEngine_MarkFailed(ADUC_DagEngineHandle handle, const char* stepId);

/**
 * @brief Mark a node as skipped.
 */
void ADUC_DagEngine_MarkSkipped(ADUC_DagEngineHandle handle, const char* stepId);

/**
 * @brief Get the current state of a node.
 */
ADUC_DagNodeState ADUC_DagEngine_GetNodeState(ADUC_DagEngineHandle handle, const char* stepId);

/**
 * @brief Check if all nodes are done, failed, or skipped.
 */
bool ADUC_DagEngine_IsComplete(ADUC_DagEngineHandle handle);

/**
 * @brief Check for cycles (returns true if cycle detected).
 */
bool ADUC_DagEngine_HasCycle(ADUC_DagEngineHandle handle);

/**
 * @brief Get count of nodes in each state.
 */
void ADUC_DagEngine_GetStats(ADUC_DagEngineHandle handle, size_t* pending, size_t* running, size_t* done, size_t* failed, size_t* skipped);

/**
 * @brief Reset all nodes to pending.
 */
void ADUC_DagEngine_Reset(ADUC_DagEngineHandle handle);

/**
 * @brief Destroy the DAG engine and free resources.
 */
void ADUC_DagEngine_Destroy(ADUC_DagEngineHandle handle);

#ifdef __cplusplus
}
#endif

#endif /* ADUC_DAG_ENGINE_H */
