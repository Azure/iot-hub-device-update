/**
 * @file dag_engine.c
 * @brief DAG scheduler implementation using topological sort.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/dag_engine.h"
#include <stdlib.h>
#include <string.h>

struct ADUC_DagNode {
    const char* stepId;
    size_t* deps;             // indices into engine->nodes
    size_t depCount;
    size_t* skipOnFailed;     // indices into engine->nodes
    size_t skipOnFailedCount;
    size_t* runOnFailed;      // indices into engine->nodes
    size_t runOnFailedCount;
    ADUC_DagNodeState state;
};

struct ADUC_DagEngine {
    ADUC_DagNode* nodes;
    size_t nodeCount;
};

static int find_node_index(const struct ADUC_DagEngine* engine, const char* stepId)
{
    for (size_t i = 0; i < engine->nodeCount; i++)
    {
        if (strcmp(engine->nodes[i].stepId, stepId) == 0)
        {
            return (int)i;
        }
    }
    return -1;
}

ADUC_Result2 ADUC_DagEngine_Create(const ADUC_DagNodeDef* nodes, size_t nodeCount, ADUC_DagEngineHandle* outHandle)
{
    if (outHandle == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_STATE, 1);
    }
    *outHandle = NULL;

    if (nodes == NULL && nodeCount > 0)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_STATE, 2);
    }

    struct ADUC_DagEngine* engine = (struct ADUC_DagEngine*)calloc(1, sizeof(struct ADUC_DagEngine));
    if (engine == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_RESOURCE, 1);
    }

    engine->nodeCount = nodeCount;
    if (nodeCount > 0)
    {
        engine->nodes = (ADUC_DagNode*)calloc(nodeCount, sizeof(ADUC_DagNode));
        if (engine->nodes == NULL)
        {
            free(engine);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_RESOURCE, 1);
        }

        // First pass: set step IDs
        for (size_t i = 0; i < nodeCount; i++)
        {
            engine->nodes[i].stepId = nodes[i].stepId;
            engine->nodes[i].state = DAG_NODE_PENDING;
        }

        // Second pass: resolve dependencies to indices
        for (size_t i = 0; i < nodeCount; i++)
        {
            size_t depCount = nodes[i].dependsOnCount;
            if (depCount > 0 && nodes[i].dependsOn != NULL)
            {
                engine->nodes[i].deps = (size_t*)calloc(depCount, sizeof(size_t));
                if (engine->nodes[i].deps == NULL)
                {
                    ADUC_DagEngine_Destroy(engine);
                    return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_RESOURCE, 1);
                }
                engine->nodes[i].depCount = depCount;

                for (size_t d = 0; d < depCount; d++)
                {
                    int idx = find_node_index(engine, nodes[i].dependsOn[d]);
                    if (idx < 0)
                    {
                        ADUC_DagEngine_Destroy(engine);
                        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_STATE, 3);
                    }
                    engine->nodes[i].deps[d] = (size_t)idx;
                }
            }

            // Resolve skipOnFailed
            size_t sofCount = nodes[i].skipOnFailedCount;
            if (sofCount > 0 && nodes[i].skipOnFailed != NULL)
            {
                engine->nodes[i].skipOnFailed = (size_t*)calloc(sofCount, sizeof(size_t));
                if (engine->nodes[i].skipOnFailed == NULL)
                {
                    ADUC_DagEngine_Destroy(engine);
                    return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_RESOURCE, 1);
                }
                engine->nodes[i].skipOnFailedCount = sofCount;

                for (size_t d = 0; d < sofCount; d++)
                {
                    int idx = find_node_index(engine, nodes[i].skipOnFailed[d]);
                    if (idx < 0)
                    {
                        ADUC_DagEngine_Destroy(engine);
                        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_STATE, 3);
                    }
                    engine->nodes[i].skipOnFailed[d] = (size_t)idx;
                }
            }

            // Resolve runOnFailed
            size_t rofCount = nodes[i].runOnFailedCount;
            if (rofCount > 0 && nodes[i].runOnFailed != NULL)
            {
                engine->nodes[i].runOnFailed = (size_t*)calloc(rofCount, sizeof(size_t));
                if (engine->nodes[i].runOnFailed == NULL)
                {
                    ADUC_DagEngine_Destroy(engine);
                    return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_RESOURCE, 1);
                }
                engine->nodes[i].runOnFailedCount = rofCount;

                for (size_t d = 0; d < rofCount; d++)
                {
                    int idx = find_node_index(engine, nodes[i].runOnFailed[d]);
                    if (idx < 0)
                    {
                        ADUC_DagEngine_Destroy(engine);
                        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_STATE, 3);
                    }
                    engine->nodes[i].runOnFailed[d] = (size_t)idx;
                }
            }
        }
    }

    *outHandle = engine;
    return ADUC_RESULT2_SUCCESS;
}

size_t ADUC_DagEngine_GetReady(ADUC_DagEngineHandle handle, const char** outStepIds, size_t maxCount)
{
    if (handle == NULL || outStepIds == NULL || maxCount == 0)
    {
        return 0;
    }

    size_t count = 0;
    for (size_t i = 0; i < handle->nodeCount && count < maxCount; i++)
    {
        if (handle->nodes[i].state != DAG_NODE_PENDING)
        {
            continue;
        }

        // Check all deps are DONE or SKIPPED
        bool allDepsDone = true;
        bool anyDepFailed = false;
        for (size_t d = 0; d < handle->nodes[i].depCount; d++)
        {
            ADUC_DagNodeState depState = handle->nodes[handle->nodes[i].deps[d]].state;
            if (depState == DAG_NODE_FAILED)
            {
                anyDepFailed = true;
                break;
            }
            if (depState != DAG_NODE_DONE && depState != DAG_NODE_SKIPPED)
            {
                allDepsDone = false;
                break;
            }
        }

        if (anyDepFailed)
        {
            // Cascade failure to this node
            handle->nodes[i].state = DAG_NODE_FAILED;
            continue;
        }

        if (!allDepsDone)
        {
            continue;
        }

        // Check skipOnFailed: if ANY referenced node is FAILED → skip
        bool shouldSkip = false;
        for (size_t s = 0; s < handle->nodes[i].skipOnFailedCount; s++)
        {
            if (handle->nodes[handle->nodes[i].skipOnFailed[s]].state == DAG_NODE_FAILED)
            {
                shouldSkip = true;
                break;
            }
        }
        if (shouldSkip)
        {
            handle->nodes[i].state = DAG_NODE_SKIPPED;
            continue;
        }

        // Check runOnFailed: only run if ANY referenced node is FAILED
        if (handle->nodes[i].runOnFailedCount > 0)
        {
            bool anyFailed = false;
            for (size_t r = 0; r < handle->nodes[i].runOnFailedCount; r++)
            {
                if (handle->nodes[handle->nodes[i].runOnFailed[r]].state == DAG_NODE_FAILED)
                {
                    anyFailed = true;
                    break;
                }
            }
            if (!anyFailed)
            {
                handle->nodes[i].state = DAG_NODE_SKIPPED;
                continue;
            }
        }

        handle->nodes[i].state = DAG_NODE_RUNNING;
        outStepIds[count++] = handle->nodes[i].stepId;
    }
    return count;
}

void ADUC_DagEngine_MarkDone(ADUC_DagEngineHandle handle, const char* stepId)
{
    if (handle == NULL || stepId == NULL)
    {
        return;
    }
    int idx = find_node_index(handle, stepId);
    if (idx >= 0)
    {
        handle->nodes[idx].state = DAG_NODE_DONE;
    }
}

void ADUC_DagEngine_MarkFailed(ADUC_DagEngineHandle handle, const char* stepId)
{
    if (handle == NULL || stepId == NULL)
    {
        return;
    }
    int idx = find_node_index(handle, stepId);
    if (idx >= 0)
    {
        handle->nodes[idx].state = DAG_NODE_FAILED;
    }
}

void ADUC_DagEngine_MarkSkipped(ADUC_DagEngineHandle handle, const char* stepId)
{
    if (handle == NULL || stepId == NULL)
    {
        return;
    }
    int idx = find_node_index(handle, stepId);
    if (idx >= 0)
    {
        handle->nodes[idx].state = DAG_NODE_SKIPPED;
    }
}

ADUC_DagNodeState ADUC_DagEngine_GetNodeState(ADUC_DagEngineHandle handle, const char* stepId)
{
    if (handle == NULL || stepId == NULL)
    {
        return DAG_NODE_PENDING;
    }
    int idx = find_node_index(handle, stepId);
    if (idx >= 0)
    {
        return handle->nodes[idx].state;
    }
    return DAG_NODE_PENDING;
}

bool ADUC_DagEngine_IsComplete(ADUC_DagEngineHandle handle)
{
    if (handle == NULL)
    {
        return true;
    }
    for (size_t i = 0; i < handle->nodeCount; i++)
    {
        if (handle->nodes[i].state == DAG_NODE_PENDING || handle->nodes[i].state == DAG_NODE_RUNNING)
        {
            return false;
        }
        /* DONE, FAILED, and SKIPPED are all terminal states */
    }
    return true;
}

// DFS cycle detection with coloring: WHITE=0, GRAY=1, BLACK=2
static bool dfs_has_cycle(const struct ADUC_DagEngine* engine, size_t nodeIdx, uint8_t* color)
{
    color[nodeIdx] = 1; // GRAY
    for (size_t d = 0; d < engine->nodes[nodeIdx].depCount; d++)
    {
        size_t dep = engine->nodes[nodeIdx].deps[d];
        if (color[dep] == 1)
        {
            return true; // back edge = cycle
        }
        if (color[dep] == 0 && dfs_has_cycle(engine, dep, color))
        {
            return true;
        }
    }
    color[nodeIdx] = 2; // BLACK
    return false;
}

bool ADUC_DagEngine_HasCycle(ADUC_DagEngineHandle handle)
{
    if (handle == NULL || handle->nodeCount == 0)
    {
        return false;
    }

    uint8_t* color = (uint8_t*)calloc(handle->nodeCount, sizeof(uint8_t));
    if (color == NULL)
    {
        return false;
    }

    bool hasCycle = false;
    for (size_t i = 0; i < handle->nodeCount; i++)
    {
        if (color[i] == 0)
        {
            if (dfs_has_cycle(handle, i, color))
            {
                hasCycle = true;
                break;
            }
        }
    }

    free(color);
    return hasCycle;
}

void ADUC_DagEngine_GetStats(ADUC_DagEngineHandle handle, size_t* pending, size_t* running, size_t* done, size_t* failed, size_t* skipped)
{
    size_t p = 0, r = 0, d = 0, f = 0, s = 0;
    if (handle != NULL)
    {
        for (size_t i = 0; i < handle->nodeCount; i++)
        {
            switch (handle->nodes[i].state)
            {
                case DAG_NODE_PENDING: p++; break;
                case DAG_NODE_RUNNING: r++; break;
                case DAG_NODE_DONE:    d++; break;
                case DAG_NODE_FAILED:  f++; break;
                case DAG_NODE_SKIPPED: s++; break;
            }
        }
    }
    if (pending) *pending = p;
    if (running) *running = r;
    if (done)    *done = d;
    if (failed)  *failed = f;
    if (skipped) *skipped = s;
}

void ADUC_DagEngine_Reset(ADUC_DagEngineHandle handle)
{
    if (handle == NULL)
    {
        return;
    }
    for (size_t i = 0; i < handle->nodeCount; i++)
    {
        handle->nodes[i].state = DAG_NODE_PENDING;
    }
}

void ADUC_DagEngine_Destroy(ADUC_DagEngineHandle handle)
{
    if (handle == NULL)
    {
        return;
    }
    for (size_t i = 0; i < handle->nodeCount; i++)
    {
        free(handle->nodes[i].deps);
        free(handle->nodes[i].skipOnFailed);
        free(handle->nodes[i].runOnFailed);
    }
    free(handle->nodes);
    free(handle);
}
