/**
 * @file logging.h
 * @brief Logging interface (stub implementation)
 */

#ifndef ADUC_LOGGING_H
#define ADUC_LOGGING_H

#include "aduc/types.h"
#include "aduc/exports.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Log a message.
 */
ADUC_SDK_EXPORT void ADUC_Log(ADUC_LogLevel level, const char* message);

#ifdef __cplusplus
}
#endif

#endif // ADUC_LOGGING_H