/**
 * @file log_writer.h
 * @brief Binary log writer for the ADU Gen2 agent.
 *
 * Implements an ETW-inspired binary logging format with compact headers
 * and variable-length payloads. Each log entry consists of an 8-byte header
 * (relative timestamp, event ID, level, flags) followed by optional payload.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_LOG_WRITER_H
#define ADUC_LOG_WRITER_H

#include "aduc/extension_types.h"

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Opaque binary log writer handle.
 */
typedef struct ADUC_LogWriter ADUC_LogWriter;

/**
 * @brief Binary log entry header (8 bytes).
 *
 * Layout:
 *   - timestamp: uint32_t — seconds since agent start (monotonic)
 *   - eventId:   uint16_t — references format string in manifest
 *   - level:     uint8_t  — ADUC_LogLevel value
 *   - flags:     uint8_t  — bit 0 = has_payload, bit 1 = has_correlation_id
 */
#define ADUC_LOG_HEADER_SIZE 8

#define ADUC_LOG_FLAG_HAS_PAYLOAD        0x01
#define ADUC_LOG_FLAG_HAS_CORRELATION_ID 0x02

/**
 * @brief Create a binary log writer.
 *
 * @param logFilePath  Path to the log file (opened for append-binary).
 * @param outWriter    Receives the created writer handle.
 * @return ADUC_Result2  Success or error code.
 */
ADUC_Result2 ADUC_Log_Create(const char* logFilePath, ADUC_LogWriter** outWriter);

/**
 * @brief Destroy a log writer, flushing any buffered data.
 *
 * @param writer  Writer to destroy (may be NULL).
 */
void ADUC_Log_Destroy(ADUC_LogWriter* writer);

/**
 * @brief Write a binary log entry.
 *
 * @param writer      Log writer handle.
 * @param level       Severity level.
 * @param eventId     Event identifier (references manifest).
 * @param payload     Binary payload data (may be NULL if payloadLen is 0).
 * @param payloadLen  Length of payload in bytes.
 * @return ADUC_Result2  Success or error code.
 */
ADUC_Result2 ADUC_Log_Write(
    ADUC_LogWriter* writer,
    ADUC_LogLevel level,
    uint16_t eventId,
    const void* payload,
    size_t payloadLen);

/**
 * @brief Write a text log entry to stderr (for console/debug mode).
 *
 * Output format: [ISO-8601 timestamp][LEVEL][component] message\n
 *
 * @param level      Severity level.
 * @param component  Component name for log prefix.
 * @param format     printf-style format string.
 * @param ...        Format arguments.
 */
void ADUC_Log_WriteText(
    ADUC_LogLevel level,
    const char* component,
    const char* format, ...)
#ifndef _MSC_VER
    __attribute__((format(printf, 3, 4)))
#endif
    ;

/**
 * @brief Write a string parameter as binary payload.
 *
 * Encodes the string as [uint16_t length][bytes] and writes it as a log entry.
 *
 * @param writer   Log writer handle.
 * @param level    Severity level.
 * @param eventId  Event identifier.
 * @param str      String to encode (must not be NULL).
 * @return ADUC_Result2  Success or error code.
 */
ADUC_Result2 ADUC_Log_WriteString(
    ADUC_LogWriter* writer,
    ADUC_LogLevel level,
    uint16_t eventId,
    const char* str);

/**
 * @brief Flush buffered log entries to disk.
 *
 * @param writer  Log writer handle.
 * @return ADUC_Result2  Success or error code.
 */
ADUC_Result2 ADUC_Log_Flush(ADUC_LogWriter* writer);

/**
 * @brief Get total bytes written by this writer.
 *
 * @param writer  Log writer handle.
 * @return Total bytes written (including buffered but unflushed data).
 */
size_t ADUC_Log_GetBytesWritten(const ADUC_LogWriter* writer);

/**
 * @brief Set the maximum log level for binary logging.
 *
 * Entries with level > maxLevel are discarded.
 *
 * @param maxLevel  Maximum level to log.
 */
void ADUC_Log_SetLevel(ADUC_LogLevel maxLevel);

/**
 * @brief Get the current maximum log level for binary logging.
 *
 * @return Current max log level.
 */
ADUC_LogLevel ADUC_Log_GetLevel(void);

/**
 * @brief Global text log level (for ADUC_Log_WriteText filtering).
 */
extern ADUC_LogLevel g_logLevel;

#ifdef __cplusplus
}
#endif

#endif // ADUC_LOG_WRITER_H
