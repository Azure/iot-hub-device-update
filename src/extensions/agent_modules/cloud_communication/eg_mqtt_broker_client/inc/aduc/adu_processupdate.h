#ifndef ADU_PROCESSUPDATE_H
#define ADU_PROCESSUPDATE_H

#include "aduc/c_utils.h"
#include "aduc/adu_upd.h"

#include <aduc/retry_utils.h>

EXTERN_C_BEGIN

void Adu_ProcessUpdate(ADUC_Update_Request_Operation_Data* updateData, ADUC_Retriable_Operation_Context* retriableOperationContext);

EXTERN_C_END

#endif // ADU_PROCESSUPDATE_H
