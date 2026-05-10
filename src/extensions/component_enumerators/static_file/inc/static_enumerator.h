/**
 * @file static_enumerator.h
 * @brief Static file-based component enumerator.
 *
 * Reads component definitions from a TOML configuration file.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_STATIC_ENUMERATOR_H
#define ADUC_STATIC_ENUMERATOR_H

#include "aduc/component_enumerator_vtable.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Get the static enumerator vtable.
 */
const ADUC_ComponentEnumeratorVtable* StaticEnumerator_GetVtable(void);

/**
 * @brief Set the path to the TOML configuration file.
 * Must be called before Enumerate.
 */
void StaticEnumerator_SetConfigPath(const char* path);

#ifdef __cplusplus
}
#endif

#endif /* ADUC_STATIC_ENUMERATOR_H */
