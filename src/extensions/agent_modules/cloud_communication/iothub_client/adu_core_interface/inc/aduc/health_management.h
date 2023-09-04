/**
 * @file health_management.h
 * @brief Implements functions that determine whether ADU Agent can function properly.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_HEALTH_MANAGEMENT_H
#define ADUC_HEALTH_MANAGEMENT_H

#include "aduc/adu_types.h"
#include <stdbool.h>

/**
 * @brief Performs necessary checks to determine whether ADU Agent can function properly.
 *
 * @return true if all checks passed.
 */
bool HealthCheck();

#endif // ADUC_HEALTH_MANAGEMENT_H
