/**
 * @file process_context.c
 * @brief Generate and parse TOML context files for child processes.
 */

#include "aduc/process_context.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <toml.h>

// Helper to write a TOML string entry (skips if value is NULL)
static void write_string_entry(FILE* fp, const char* key, const char* value)
{
    if (value != NULL)
    {
        fprintf(fp, "%s = \"%s\"\n", key, value);
    }
    else
    {
        fprintf(fp, "%s = \"\"\n", key);
    }
}

ADUC_Result2 ADUC_ProcessContext_Write(const char* filePath, const ADUC_ProcessContextData* data)
{
    if (filePath == NULL || data == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    FILE* fp = fopen(filePath, "w");
    if (fp == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 2);
    }

    fprintf(fp, "[workflow]\n");
    write_string_entry(fp, "workflow_id", data->workflowId);
    write_string_entry(fp, "deployment_id", data->deploymentId);
    write_string_entry(fp, "step_id", data->stepId);
    write_string_entry(fp, "handler_type", data->handlerType);
    write_string_entry(fp, "installed_criteria", data->installedCriteria);
    fprintf(fp, "step_index = %u\n", data->stepIndex);
    fprintf(fp, "total_steps = %u\n", data->totalSteps);
    fprintf(fp, "\n");

    fprintf(fp, "[paths]\n");
    write_string_entry(fp, "work_folder", data->workFolder);
    write_string_entry(fp, "content_dir", data->contentDir);
    write_string_entry(fp, "ipc_socket_path", data->ipcSocketPath);
    fprintf(fp, "\n");

    fprintf(fp, "[component]\n");
    write_string_entry(fp, "component_id", data->componentId);
    write_string_entry(fp, "component_group", data->componentGroup);

    fclose(fp);
    return ADUC_RESULT2_SUCCESS;
}

// Helper: duplicate a TOML string value (returns empty string dup if key missing)
static char* toml_strdup_key(toml_table_t* table, const char* key)
{
    toml_datum_t datum = toml_string_in(table, key);
    if (datum.ok)
    {
        return datum.u.s; // already malloc'd by tomlc99
    }
    return strdup("");
}

ADUC_Result2 ADUC_ProcessContext_Read(const char* filePath, ADUC_ProcessContextData* outData)
{
    if (filePath == NULL || outData == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    memset(outData, 0, sizeof(*outData));

    FILE* fp = fopen(filePath, "r");
    if (fp == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 2);
    }

    char errbuf[256];
    toml_table_t* root = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (root == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 3);
    }

    // [workflow] section
    toml_table_t* workflow = toml_table_in(root, "workflow");
    if (workflow != NULL)
    {
        outData->workflowId = toml_strdup_key(workflow, "workflow_id");
        outData->deploymentId = toml_strdup_key(workflow, "deployment_id");
        outData->stepId = toml_strdup_key(workflow, "step_id");
        outData->handlerType = toml_strdup_key(workflow, "handler_type");
        outData->installedCriteria = toml_strdup_key(workflow, "installed_criteria");

        toml_datum_t si = toml_int_in(workflow, "step_index");
        if (si.ok)
        {
            outData->stepIndex = (uint32_t)si.u.i;
        }
        toml_datum_t ts = toml_int_in(workflow, "total_steps");
        if (ts.ok)
        {
            outData->totalSteps = (uint32_t)ts.u.i;
        }
    }

    // [paths] section
    toml_table_t* paths = toml_table_in(root, "paths");
    if (paths != NULL)
    {
        outData->workFolder = toml_strdup_key(paths, "work_folder");
        outData->contentDir = toml_strdup_key(paths, "content_dir");
        outData->ipcSocketPath = toml_strdup_key(paths, "ipc_socket_path");
    }

    // [component] section
    toml_table_t* component = toml_table_in(root, "component");
    if (component != NULL)
    {
        outData->componentId = toml_strdup_key(component, "component_id");
        outData->componentGroup = toml_strdup_key(component, "component_group");
    }

    toml_free(root);
    return ADUC_RESULT2_SUCCESS;
}

void ADUC_ProcessContext_Free(ADUC_ProcessContextData* data)
{
    if (data == NULL)
    {
        return;
    }

    free((void*)data->workflowId);
    free((void*)data->deploymentId);
    free((void*)data->stepId);
    free((void*)data->handlerType);
    free((void*)data->workFolder);
    free((void*)data->contentDir);
    free((void*)data->ipcSocketPath);
    free((void*)data->installedCriteria);
    free((void*)data->componentId);
    free((void*)data->componentGroup);

    memset(data, 0, sizeof(*data));
}
