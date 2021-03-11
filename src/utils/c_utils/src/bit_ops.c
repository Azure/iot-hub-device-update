/**
 * @file bit_ops.c
 * @brief This file contains extern declaration for each extern inline function from the header that manipulate bits.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/bit_ops.h"

/*
 * C99 extern inline model where this compilation unit has the externally visible code when not inlining (e.g. no optimizations switch enabled).
 * When compiler decides to inline, these become redundant declarations so we suppress that check.
 */

// NOLINTNEXTLINE(readability-redundant-declaration)
extern /* inline */ _Bool BitOps_AreAllBitsSet(size_t bits, size_t mask);
