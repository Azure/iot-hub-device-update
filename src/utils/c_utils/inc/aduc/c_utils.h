/**
 * @file c_utils.h
 * @brief Helpers and macros for C code.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_C_UTILS_H
#define ADUC_C_UTILS_H

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

#endif // ADUC_C_UTILS_H
