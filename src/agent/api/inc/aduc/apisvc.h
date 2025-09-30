/**
 * @file apisvc.h
 * @brief The header for service handling incoming cross-process API calls.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_APISVC_H_
#define ADUC_APISVC_H_

#include "aduc/c_utils.h"

#include <stdbool.h>

EXTERN_C_BEGIN

bool init_api_svc(const char* fifoPath);
bool uninit_api_svc();

EXTERN_C_END

#endif // ADUC_APISVC_H_
