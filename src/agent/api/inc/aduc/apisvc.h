/**
 * @file apisvc.h
 * @brief The header for service handling incoming cross-process API calls.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_APISVC_H_
#define ADUC_APISVC_H_

#include <stdbool.h>

bool init_api_svc();
void unint_api_svc();

#endif // ADUC_APISVC_H_
