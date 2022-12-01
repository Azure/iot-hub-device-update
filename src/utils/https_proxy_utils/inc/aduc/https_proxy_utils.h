/**
 * @file https_proxy_utils.h
 * @brief inline functions that for initialize and free IOTHUB_PROXY_OPTIONS struct.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __HTTPS_PROXY_UTILS_H__
#define __HTTPS_PROXY_UTILS_H__

#include "aduc/c_utils.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include <stdbool.h> // for bool

EXTERN_C_BEGIN

/**
 * @brief Initializes @p proxyOptions object by reading and parsing environment variable 'https_proxy' or 'HTTPS_PROXY', in that order.
 * @param proxyOptions HTTP_PROXY_OPTIONS A pointer to HTTP_PROXY_OPTIONS struct to be initialized.
 *                     Caller must call UninitializeProxyOptions() to free memory allocated for the member variables.
 * @return bool True when the proxy options successfully initialized.
 */
bool InitializeProxyOptions(HTTP_PROXY_OPTIONS* proxyOptions);

/**
 * @brief Frees all memory allocated for @p proxyOptions member variables.
 *
 * @param proxyOptions HTTP_PROXY_OPTIONS An object to uninitialize.
 */
void UninitializeProxyOptions(HTTP_PROXY_OPTIONS* proxyOptions);

EXTERN_C_END

#endif /* __HTTPS_PROXY_UTILS_H__ */
