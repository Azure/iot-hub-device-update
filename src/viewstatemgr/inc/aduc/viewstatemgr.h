#ifndef ADUC_VIEWSTATEMGR_H
#define ADUC_VIEWSTATEMGR_H

#include <aduc/aducsdk.h>
#include <aduc/c_utils.h>
#include <aduc/result.h>

#include <pthread.h>
#include <stdbool.h>

EXTERN_C_BEGIN

typedef struct tagViewStateManager
{
    bool initialized;
    ADUC_ServiceStatus svc_stat;
    pthread_mutex_t mut;
} ViewStateManager;

int viewstatemgr_create(ViewStateManager* vsm);
void viewstatemgr_destroy(ViewStateManager* vsm);

bool viewstatemgr_svcstatus_get(ViewStateManager* vsm, ADUC_ServiceStatus* out_status);
bool viewstatemgr_svcstatus_set(ViewStateManager* vsm, ADUC_ServiceStatus status);

EXTERN_C_END

#endif // ADUC_VIEWSTATEMGR_H
