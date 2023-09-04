/**
 * @file agent_common.h
 * @brief Helpers and macros for C code.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_COMMON_H
#    define ADUC_COMMON_H

#    ifdef __cplusplus
#        define EXTERN_C_BEGIN \
            extern "C"         \
            {
#        define EXTERN_C_END }
#    else
#        define EXTERN_C_BEGIN
#        define EXTERN_C_END
#    endif

/**
 * @brief Explicitly states that a parameter is not used in this scope.
 */
#    ifndef IGNORED_PARAMETER
#        define IGNORED_PARAMETER(param) (void)(param)
#    endif

#endif // ADUC_COMMON_H
