/**
 * @file aduc_banned.h
 * @brief Macro definitions that cause banned functions considered to be
 * unsafe to result in a compilation error.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_BANNED_H
#define ADUC_BANNED_H

#define DEFINE_BANNED_ALTERNATIVE(BANNED_FUNC, ALT_FUNC) BANNED_FUNC##_is_banned__Please_use_##ALT_FUNC##_instead

#undef strncpy
#define strncpy(dest,src,n) DEFINE_BANNED_ALTERNATIVE(strncpy, ADUC_Safe_StrCopyN)

#endif
