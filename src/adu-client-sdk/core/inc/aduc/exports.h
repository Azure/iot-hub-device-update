/**
 * @file exports.h
 * @brief Azure Device Update Core SDK Export Definitions
 *
 * Platform-specific export/import macros for building shared libraries.
 *
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_EXPORTS_H
#define ADUC_EXPORTS_H

// Platform-specific export/import macros
#ifdef _WIN32
    #ifdef ADUC_SDK_EXPORTS
        #define ADUC_SDK_EXPORT __declspec(dllexport)
    #else
        #define ADUC_SDK_EXPORT __declspec(dllimport)
    #endif
    #define ADUC_SDK_LOCAL
#else
    #if defined(ADUC_SDK_EXPORTS) && __GNUC__ >= 4
        #define ADUC_SDK_EXPORT __attribute__((visibility("default")))
        #define ADUC_SDK_LOCAL  __attribute__((visibility("hidden")))
    #else
        #define ADUC_SDK_EXPORT
        #define ADUC_SDK_LOCAL
    #endif
#endif

// C linkage for C++ compatibility
#ifdef __cplusplus
    #define ADUC_SDK_C_LINKAGE extern "C"
#else
    #define ADUC_SDK_C_LINKAGE
#endif

// Extension entry point macros
#define ADUC_SDK_EXPORT_C ADUC_SDK_C_LINKAGE ADUC_SDK_EXPORT

// Calling convention macros
#ifdef _WIN32
    #define ADUC_SDK_CALL __stdcall
#else
    #define ADUC_SDK_CALL
#endif

#endif // ADUC_EXPORTS_H
