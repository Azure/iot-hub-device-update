/**
 * @file viewstatemgr.c
 * @brief The implementation for view state manager.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/viewstatemgr.h"
#include "aduc/logging.h"
#include "aduc/result.h"
#include "aduc/types/adu_core.h"

#include <pthread.h>
#include <string.h>

int viewstatemgr_create(ViewStateManager* vsm)
{
    Log_Debug("viewstatemgr_create called");
    if (vsm == NULL)
    {
        return -1;
    }

    if (pthread_mutex_init(&vsm->mut, NULL) != 0)
    {
        return -2;
    }

    vsm->svc_stat = ADUC_ServiceStatus_Initializing;
    vsm->initialized = true;

    Log_Info("Successfully created ViewStateManager instance.");
    return 0;
}

void viewstatemgr_destroy(ViewStateManager* vsm)
{
    Log_Debug("viewstatemgr_destroy called");
    if (vsm != NULL && vsm->initialized)
    {
        Log_Info("Destroying ViewStateManager instance.");
        pthread_mutex_destroy(&vsm->mut);
        memset(vsm, 0, sizeof(ViewStateManager));
    }
}

bool viewstatemgr_svcstatus_get(ViewStateManager* vsm, ADUC_ServiceStatus* out_status)
{
    Log_Debug("viewstatemgr_get called");
    if (out_status == NULL)
    {
        return false;
    }

    if (!vsm->initialized)
    {
        Log_Warn("viewstatemgr_svcstatus_get: vsm not initialized");
        return false;
    }

    pthread_mutex_lock(&vsm->mut);
    *out_status = vsm->svc_stat;
    pthread_mutex_unlock(&vsm->mut);
    Log_Debug("svcstatus_get returning status %d", *out_status);
    return true;
}

bool viewstatemgr_svcstatus_set(ViewStateManager* vsm, ADUC_ServiceStatus new_status)
{
    if (vsm == NULL)
    {
        return false;
    }
    pthread_mutex_lock(&vsm->mut);
    vsm->svc_stat = new_status;
    pthread_mutex_unlock(&vsm->mut);
    Log_Debug("svcstatus_set set new status to %d", new_status);
    return true;
}
