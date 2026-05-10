/**
 * @file workflow_persist.c
 * @brief Workflow state persistence (binary file with atomic rename).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/workflow_persist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PERSIST_MAGIC 0x41445550  /* "ADUP" */
#define PERSIST_VERSION 1
#define PERSIST_FILENAME "workflow.state"
#define PERSIST_TMP_FILENAME "workflow.state.tmp"

typedef struct PersistHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t dataSize;
} PersistHeader;

static void build_path(char* buf, size_t bufSize, const char* dir, const char* filename)
{
    size_t dirLen = strlen(dir);
    if (dirLen > 0 && (dir[dirLen - 1] == '/' || dir[dirLen - 1] == '\\'))
    {
        snprintf(buf, bufSize, "%s%s", dir, filename);
    }
    else
    {
        snprintf(buf, bufSize, "%s/%s", dir, filename);
    }
}

ADUC_Result2 ADUC_WorkflowPersist_Save(const char* stateDir, const ADUC_WorkflowState2* state)
{
    if (stateDir == NULL || state == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_IO, 1);
    }

    char tmpPath[512];
    char finalPath[512];
    build_path(tmpPath, sizeof(tmpPath), stateDir, PERSIST_TMP_FILENAME);
    build_path(finalPath, sizeof(finalPath), stateDir, PERSIST_FILENAME);

    FILE* f = fopen(tmpPath, "wb");
    if (f == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_IO, 2);
    }

    PersistHeader header;
    header.magic = PERSIST_MAGIC;
    header.version = PERSIST_VERSION;
    header.dataSize = (uint32_t)sizeof(ADUC_WorkflowState2);

    size_t written = fwrite(&header, sizeof(header), 1, f);
    if (written != 1)
    {
        fclose(f);
        remove(tmpPath);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_IO, 3);
    }

    written = fwrite(state, sizeof(ADUC_WorkflowState2), 1, f);
    if (written != 1)
    {
        fclose(f);
        remove(tmpPath);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_IO, 3);
    }

    fclose(f);

    // Atomic rename for crash safety
    remove(finalPath);
    if (rename(tmpPath, finalPath) != 0)
    {
        remove(tmpPath);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_IO, 4);
    }

    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_WorkflowPersist_Load(const char* stateDir, ADUC_WorkflowState2* outState)
{
    if (stateDir == NULL || outState == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_IO, 1);
    }

    memset(outState, 0, sizeof(ADUC_WorkflowState2));

    char path[512];
    build_path(path, sizeof(path), stateDir, PERSIST_FILENAME);

    FILE* f = fopen(path, "rb");
    if (f == NULL)
    {
        // No state file — return success with zeroed state
        return ADUC_RESULT2_SUCCESS;
    }

    PersistHeader header;
    if (fread(&header, sizeof(header), 1, f) != 1)
    {
        fclose(f);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_IO, 5);
    }

    if (header.magic != PERSIST_MAGIC || header.version != PERSIST_VERSION)
    {
        fclose(f);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_IO, 6);
    }

    if (header.dataSize != (uint32_t)sizeof(ADUC_WorkflowState2))
    {
        fclose(f);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_IO, 7);
    }

    if (fread(outState, sizeof(ADUC_WorkflowState2), 1, f) != 1)
    {
        fclose(f);
        memset(outState, 0, sizeof(ADUC_WorkflowState2));
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_IO, 5);
    }

    fclose(f);
    return ADUC_RESULT2_SUCCESS;
}

bool ADUC_WorkflowPersist_HasState(const char* stateDir)
{
    if (stateDir == NULL)
    {
        return false;
    }

    char path[512];
    build_path(path, sizeof(path), stateDir, PERSIST_FILENAME);

    FILE* f = fopen(path, "rb");
    if (f == NULL)
    {
        return false;
    }
    fclose(f);
    return true;
}

ADUC_Result2 ADUC_WorkflowPersist_Clear(const char* stateDir)
{
    if (stateDir == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_WORKFLOW, ADUC_CATEGORY_IO, 1);
    }

    char path[512];
    build_path(path, sizeof(path), stateDir, PERSIST_FILENAME);

    // remove returns 0 on success or if file doesn't exist (on most platforms)
    remove(path);

    // Also clean up any leftover tmp file
    char tmpPath[512];
    build_path(tmpPath, sizeof(tmpPath), stateDir, PERSIST_TMP_FILENAME);
    remove(tmpPath);

    return ADUC_RESULT2_SUCCESS;
}
