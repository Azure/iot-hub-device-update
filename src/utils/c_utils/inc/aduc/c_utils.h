/**
 * @file c_utils.h
 * @brief Helpers and macros for C code.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_C_UTILS_H
#    define ADUC_C_UTILS_H

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
 * @brief Gets the size in elements of a statically allocated array.
 * Do not use on arrays that were allocated on the heap.
 */
#    define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/**
 * @brief Explicitly states that a parameter is not used in this scope.
 */
#    ifndef UNREFERENCED_PARAMETER
#        define UNREFERENCED_PARAMETER(param) (void)(param)
#    endif

/**
 * @brief Defines the __attribute__ value for non GNUC compilers
 */
#    ifndef __GNUC__
#        define __attribute__(x)
#    endif

/**
 * @brief Compile-time assertion that some boolean expression @param e is true.
 */
#    ifndef STATIC_ASSERT

#        define STATIC_ASSERT(e) typedef char __STATIC_ASSERT__[(e) ? 1 : -1] __attribute__((unused))

#    endif

#endif // ADUC_C_UTILS_H
