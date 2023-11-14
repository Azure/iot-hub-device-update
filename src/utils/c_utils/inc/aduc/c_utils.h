/**
 * @file c_utils.h
 * @brief Helpers and macros for C code.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_C_UTILS_H
#define ADUC_C_UTILS_H

#include <stdlib.h> // calloc

#ifdef __cplusplus
#    define EXTERN_C_BEGIN \
        extern "C"         \
        {
#    define EXTERN_C_END }
#else
#    define EXTERN_C_BEGIN
#    define EXTERN_C_END
#endif

/**
 * @brief Gets the size in elements of a statically allocated array.
 * Do not use on arrays that were allocated on the heap.
 */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/**
 * @brief Explicitly states that a parameter is not used in this scope.
 */
#ifndef UNREFERENCED_PARAMETER
#    define UNREFERENCED_PARAMETER(param) (void)(param)
#endif

/**
 * @brief Defines the __attribute__ value for non GNUC compilers
 */
#ifndef __GNUC__
#    define __attribute__(x)
#endif

/**
 * @brief Defines an export declspec for exporting methods.
 */
#if defined(WIN32)
#    define EXPORTED_METHOD __declspec(dllexport)
#else
#    define EXPORTED_METHOD
#endif

/**
 * @brief Compile-time assertion that some boolean expression @param e is true.
 */
#ifndef STATIC_ASSERT
#    define STATIC_ASSERT(e) typedef char __STATIC_ASSERT__[(e) ? 1 : -1] __attribute__((unused))
#endif

/**
 * @brief Calls calloc for a single element of type *(PTR) element_type and assigns to assignee.
 * If calloc returns NULL, it goes to done label.
 * @details Example Usage:
 * MyType* my_fn()
 * {
 *     MyType* ret = NULL;
 * ...
 *     MyType* my_var = NULL;
 * ...
 *     ADUC_ALLOC(my_Var)
 * ...
 *     ret = my_var;
 *     my_var = NULL; // transfer ownership
 * done:
 *     free(my_var);
 *     return ret;
 * }
 */
#define ADUC_ALLOC(PTR) \
    (PTR) = calloc(1, sizeof(*(PTR))); \
    if ((PTR) == NULL)                 \
    {                                  \
        goto done;                     \
    }

/**
 * @brief Calls calloc with specified number of elements and element byte size and assigns to assignee.
 * If calloc returns NULL, it goes to done label.
 *
 */
#define ADUC_ALLOC_BLOCK(assignee, num_elements, element_byte_size) \
    (assignee) = calloc((num_elements), (element_byte_size)); \
    if ((assignee) == NULL)       \
    {                             \
        goto done;                \
    }


#endif // ADUC_C_UTILS_H
