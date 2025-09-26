#ifndef ADUC_VIEWSTATEMGR_H
#define ADUC_VIEWSTATEMGR_H

#include <aduc/aducsdk.h>
#include <aduc/c_utils.h>
#include <aduc/result.h>

EXTERN_C_BEGIN

typedef void* ViewStateMgrHandle;
ViewStateMgrHandle viewstatemgr_create();
void viewstatemgr_destroy(ViewStateMgrHandle h);

ADUC_Result viewstatemgr_svcstatus_get(ViewStateMgrHandle h, ADUC_ServiceStatus* out_status);
ADUC_Result viewstatemgr_svcstatus_set(ViewStateMgrHandle h, ADUC_ServiceStatus status);

extern ViewStateMgrHandle g_viewstatemgr_handle; // located in agent/main.c

EXTERN_C_END

#endif // ADUC_VIEWSTATEMGR_H
