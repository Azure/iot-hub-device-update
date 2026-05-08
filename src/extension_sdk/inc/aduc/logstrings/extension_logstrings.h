/**
 * @file extension_logstrings.h
 * @brief Log format strings for extension loader and registry events.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_EXTENSION_LOGSTRINGS_H
#define ADUC_EXTENSION_LOGSTRINGS_H

#define ADUC_LOG_EXT_SCAN_DIR           "Scanning extension directory: %s"
#define ADUC_LOG_EXT_FOUND              "Found extension: %s (type=%d, version=%s)"
#define ADUC_LOG_EXT_DLOPEN             "Loading shared library: %s"
#define ADUC_LOG_EXT_DLOPEN_FAIL        "Failed to load shared library: %s, error=%s"
#define ADUC_LOG_EXT_NO_DESCRIPTOR      "No ADUC_GetExtensionDescriptor symbol in: %s"
#define ADUC_LOG_EXT_REGISTERED         "Extension registered: name=%s, type=%d, caps=[%s]"
#define ADUC_LOG_EXT_UNLOADED           "Extension unloaded: %s"
#define ADUC_LOG_EXT_VTABLE_INVALID     "Extension vtable validation failed: %s, missing=%s"

#endif // ADUC_EXTENSION_LOGSTRINGS_H
