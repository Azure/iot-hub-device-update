/**
 * @file aduc_inode.h
 * @brief ADU client macro definitions related to inodes
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_INODE_H
#define ADUC_INODE_H

// inode values start at 1. 0 is a sentinel inode value analogous to NULL for pointers.
// When saving inode value of a file, this sentinel value is used to indicate that no
// inode has been saved yet.
#define ADUC_INODE_SENTINEL_VALUE 0

#endif // ADUC_INODE_H
