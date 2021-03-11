/**
 * @file bit_ops.h
 * @brief inline functions that manipulate bits.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __BIT_OPS_H__
#define __BIT_OPS_H__

#include <stdbool.h> // for _Bool
#include <stddef.h> // for size_t

/*
 * This is using C99 extern inline model--inline definition is in header and extern declaration is in exactly one
 * complication unit (see bit_ops.c).
 */

/**
 * @brief Does bitwise-AND filtering on bits using the mask and ensures that all the high bits of the mask were set.
 * @param bits The bits to test with mask.
 * @param mask The bitmask to apply.
 * @return true when all bits are high for corresponding high bits in the mask.
 */
inline _Bool BitOps_AreAllBitsSet(size_t bits, size_t mask)
{
    return (bits & mask) == mask;
}

#endif /* __BIT_OPS_H__ */
