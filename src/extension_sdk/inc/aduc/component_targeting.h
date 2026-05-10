/**
 * @file component_targeting.h
 * @brief Evaluate property-based targeting expressions against components.
 *
 * Targeting criteria format:
 *   "group=firmware"
 *   "manufacturer=Contoso AND model=Widget-1000"
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_COMPONENT_TARGETING_H
#define ADUC_COMPONENT_TARGETING_H

#include "aduc/component_enumerator_vtable.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Evaluate a targeting expression against a component. */
bool ADUC_Targeting_Matches(const ADUC_ComponentInfo* component, const char* expression);

/** Parse and validate a targeting expression (returns false if syntax error). */
bool ADUC_Targeting_IsValid(const char* expression);

#ifdef __cplusplus
}
#endif

#endif /* ADUC_COMPONENT_TARGETING_H */
