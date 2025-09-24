#include "aduc/viewstatemgr.h"
#include "aduc/result.h"
#include "aduc/logging.h"
#include "aduc/types/adu_core.h"

#include <pthread.h>
#include <string.h>
#include <string.h>

pthread_mutex_t g_viewstatemgr_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct tagViewStateMgrStore
{
    bool initialized;
    ADUC_ServiceStatus viewState;
} ViewStateMgr;

ViewStateMgr g_viewstatemgr_state = {0};

ViewStateMgrHandle viewstatemgr_create()
{
    g_viewstatemgr_state.initialized = true;
    g_viewstatemgr_state.viewState = ADUC_ServiceStatus_None;
    return &g_viewstatemgr_state;
}

void viewstatemgr_destroy(ViewStateMgrHandle h)
{
    pthread_mutex_lock(&g_viewstatemgr_mutex);
    memset(h, 0, sizeof(ViewStateMgr));
    pthread_mutex_unlock(&g_viewstatemgr_mutex);
}

ADUC_Result viewstatemgr_svcstatus_get(ViewStateMgrHandle h, ADUC_ServiceStatus* out_status)
{
    ADUC_Result result = {0};
    if (h == NULL || out_status == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_INVALIDARG;
        return result;
    }
    if (!g_viewstatemgr_state.initialized)
    {
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_UNINITIALIZED;
        return result;
    }

    pthread_mutex_lock(&g_viewstatemgr_mutex);
    *out_status = g_viewstatemgr_state.viewState;
    pthread_mutex_unlock(&g_viewstatemgr_mutex);
    result.ResultCode = ADUC_Result_Success;
    return result;
}

ADUC_Result viewstatemgr_svcstatus_set(ViewStateMgrHandle h, ADUC_ServiceStatus new_status)
{
    pthread_mutex_lock(&g_viewstatemgr_mutex);
    g_viewstatemgr_state.viewState = new_status;
    pthread_mutex_unlock(&g_viewstatemgr_mutex);
    return (ADUC_Result){ .ResultCode = ADUC_Result_Success, .ExtendedResultCode = 0 };
}
