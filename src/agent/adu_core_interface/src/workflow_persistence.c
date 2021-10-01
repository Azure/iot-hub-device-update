/*
 * @file workflow_persistence.c
 * @brief Implementation for serialization and deserialization of minimal data for startup logic and idle reporting.
 *
 * @copyright Copyright (c) Microsoft Corp.
 */
#include "aduc/types/workflow.h"
#include "aduc/workflow_utils.h"
#include "parson_json_utils.h"
#include "workflow_persistence.h"
#include <parson.h>
#include <stdlib.h>


// fwd decl
JSON_Value* GetReportingJsonValue(ADUC_WorkflowData* workflowData, ADUCITF_State updateState, const ADUC_Result* result, const char* installedUpdateId);
char* workflow_get_expected_update_id_string(void* WorkflowHandle);

#define WORKFLOWSTEP_PERSISTENCE_FIELD_NAME "WorkflowStep"
#define RESULTCODE_PERSISTENCE_FIELD_NAME "ResultCode"
#define EXTENDEDRESULTCODE_PERSISTENCE_FIELD_NAME "ExtendedResultCode"
#define SYSTEMREBOOTSTATE_PERSISTENCE_FIELD_NAME "SystemRebootState"
#define AGENTRESTARTSTATE_PERSISTENCE_FIELD_NAME "AgentRestartState"

#define EXPECTEDUPDATEID_PERSISTENCE_FIELD_NAME "ExpectedUpdateID"
#define WORKFLOWID_PERSISTENCE_FIELD_NAME "WorkflowId"
#define WORKFOLDER_PERSISTENCE_FIELD_NAME "WorkFolder"
#define REPORTINGJSON_PERSISTENCE_FIELD_NAME "ReportingJson"

/**
 * @brief Serializes workflow persistence state needed for startup logic and reporting extracted to the file system
 * @param workflowData[in] The workflow data used.
 * @return boolean success.
 */
bool WorkflowPersistence_Serialize(ADUC_WorkflowData* workflowData)
{
    bool result = false;
    char* reportingSerialized = NULL;
    char* workFolder = NULL;
    JSON_Value* reportingJsonValue = NULL;

    JSON_Value* rootValue = json_value_init_object();
    JSON_Object* object = json_value_get_object(rootValue);
    JSON_Status status;
    char* expectedUpdateId = NULL;

    status = json_object_set_number(object, WORKFLOWSTEP_PERSISTENCE_FIELD_NAME, workflow_get_current_workflowstep(workflowData->WorkflowHandle));
    if (status != JSONSuccess)
    {
        goto done;
    }

    status = json_object_set_number(object, RESULTCODE_PERSISTENCE_FIELD_NAME, workflowData->Result.ResultCode);
    if (status != JSONSuccess)
    {
        goto done;
    }

    status = json_object_set_number(object, EXTENDEDRESULTCODE_PERSISTENCE_FIELD_NAME, workflowData->Result.ExtendedResultCode);
    if (status != JSONSuccess)
    {
        goto done;
    }

    status = json_object_set_number(object, SYSTEMREBOOTSTATE_PERSISTENCE_FIELD_NAME, workflowData->SystemRebootState);
    if (status != JSONSuccess)
    {
        goto done;
    }

    status = json_object_set_number(object, AGENTRESTARTSTATE_PERSISTENCE_FIELD_NAME, workflowData->AgentRestartState);
    if (status != JSONSuccess)
    {
        goto done;
    }

    expectedUpdateId = workflow_get_expected_update_id_string(workflowData->WorkflowHandle);
    if (expectedUpdateId == NULL)
    {
        goto done;
    }

    status = json_object_set_string(object, EXPECTEDUPDATEID_PERSISTENCE_FIELD_NAME, expectedUpdateId);
    if (status != JSONSuccess)
    {
        goto done;
    }

    status = json_object_set_string(object, WORKFLOWID_PERSISTENCE_FIELD_NAME, workflow_peek_id(workflowData->WorkflowHandle));
    if (status != JSONSuccess)
    {
        goto done;
    }

    workFolder = workflow_get_workfolder(workflowData->WorkflowHandle);
    if (workFolder == NULL)
    {
        goto done;
    }

    status = json_object_set_string(object, WORKFOLDER_PERSISTENCE_FIELD_NAME, workFolder);
    if (status != JSONSuccess)
    {
        goto done;
    }

    const ADUCITF_State rootState = workflow_get_root_state(workflowData->WorkflowHandle);
    const ADUC_Result rootResult = workflow_get_result(workflowData->WorkflowHandle);
    reportingJsonValue = GetReportingJsonValue(workflowData, rootState, &rootResult, expectedUpdateId);
    if (reportingJsonValue == NULL)
    {
        goto done;
    }

    reportingSerialized = json_serialize_to_string(reportingJsonValue);
    if (reportingSerialized == NULL)
    {
        goto done;
    }

    status = json_object_set_string(object, REPORTINGJSON_PERSISTENCE_FIELD_NAME, reportingSerialized);
    if (status != JSONSuccess)
    {
        goto done;
    }

    status = json_serialize_to_file_pretty(rootValue, ADUC_WORKFLOWPERSISTENCE_FILE_PATH);
    if (status != JSONSuccess)
    {
        goto done;
    }

    result = true;

done:
    if (!result)
    {
        Log_Error("Failed to persist workflow state!");
    }

    free(workFolder);
    free(expectedUpdateId);
    json_free_serialized_string(reportingSerialized);
    json_value_free(reportingJsonValue);
    json_value_free(rootValue);

    return result;
}

/**
 * @brief deserializes workflow persistence state from the file system.
 * @return The pointer to WorkflowPersistenceState. It must be freed by caller by calling WorkflowPersistence_Free
 */
WorkflowPersistenceState* WorkflowPersistence_Deserialize()
{
    JSON_Value* rootValue = NULL;
    WorkflowPersistenceState* result = NULL;
    WorkflowPersistenceState* state = (WorkflowPersistenceState*)malloc(sizeof(WorkflowPersistenceState));
    if (state == NULL)
    {
        goto done;
    }

    rootValue = json_parse_file(ADUC_WORKFLOWPERSISTENCE_FILE_PATH);
    if (rootValue == NULL)
    {
        goto done;
    }

    JSON_Object* object = json_value_get_object(rootValue);

    if (!json_object_has_value(object, WORKFLOWSTEP_PERSISTENCE_FIELD_NAME))
    {
        goto done;
    }
    state->WorkflowStep = json_object_get_number(object, WORKFLOWSTEP_PERSISTENCE_FIELD_NAME);

    if (!json_object_has_value(object, RESULTCODE_PERSISTENCE_FIELD_NAME))
    {
        goto done;
    }
    (state->Result).ResultCode = json_object_get_number(object, RESULTCODE_PERSISTENCE_FIELD_NAME);

    if (!json_object_has_value(object, EXTENDEDRESULTCODE_PERSISTENCE_FIELD_NAME))
    {
        goto done;
    }
    (state->Result).ExtendedResultCode = json_object_get_number(object, EXTENDEDRESULTCODE_PERSISTENCE_FIELD_NAME);

    if (!json_object_has_value(object, SYSTEMREBOOTSTATE_PERSISTENCE_FIELD_NAME))
    {
        goto done;
    }
    state->SystemRebootState = json_object_get_number(object, SYSTEMREBOOTSTATE_PERSISTENCE_FIELD_NAME);

    if (!json_object_has_value(object, AGENTRESTARTSTATE_PERSISTENCE_FIELD_NAME))
    {
        goto done;
    }
    state->AgentRestartState = json_object_get_number(object, AGENTRESTARTSTATE_PERSISTENCE_FIELD_NAME);

    if (!ADUC_JSON_GetStringField(rootValue, EXPECTEDUPDATEID_PERSISTENCE_FIELD_NAME, &(state->ExpectedUpdateId)))
    {
        goto done;
    }

    if (!ADUC_JSON_GetStringField(rootValue, WORKFLOWID_PERSISTENCE_FIELD_NAME, &(state->WorkflowId)))
    {
        goto done;
    }

    if (!ADUC_JSON_GetStringField(rootValue, WORKFOLDER_PERSISTENCE_FIELD_NAME, &(state->WorkFolder)))
    {
        goto done;
    }

    if (!ADUC_JSON_GetStringField(rootValue, REPORTINGJSON_PERSISTENCE_FIELD_NAME, &(state->ReportingJson)))
    {
        goto done;
    }

    result = state;
    state = NULL;

done:
    if (result == NULL)
    {
        Log_Info("Failed to deserialize workflow state.");
    }

    json_value_free(rootValue);
    free(state);

    return result;
}

/**
 * @brief Frees the persistence state
 * @param persistenceState[in,out] The @p WorkflowPersistenceState pointer to be freed.
 */
void WorkflowPersistence_Free(WorkflowPersistenceState* persistenceState)
{
    if (persistenceState)
    {
        free(persistenceState->ExpectedUpdateId);
        free(persistenceState->WorkflowId);
        free(persistenceState->WorkFolder);
        free(persistenceState->ReportingJson);

        free(persistenceState);
    }
}