/**
 * @file command_helper.h
 * @brief A helper library for inter-agent commands support.
 *
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */

#ifndef ADUC_COMMAND_HELPER_H
#define ADUC_COMMAND_HELPER_H

/**
 * @brief Callback method for a command.
 *
 * @param workCompletionData Method and value to call when work completes if *Result_InProgress returned.
 * @param workflowData Data about what to download.
 */
typedef _Bool (*ADUC_CommandCallbackFunc)(const char* command, void* commandContext);

/**
 * @brief A struct containing a basic command information.
 */
typedef struct _tagADUC_Command
{
    const char* commandText; /**< command text */
    ADUC_CommandCallbackFunc callback; /**< callback function for the command */
} ADUC_Command;

/**
 * @brief Send specified @p command to the main Device Update agent process.
 *
 * @param command A command to send.
 *
 * @return _Bool Returns true if success.
 */
_Bool SendCommand(const char* command);

/**
 * @brief Initialize command listener thread.
 */
_Bool InitializeCommandListenerThread();

/**
 * @brief Uninitialize command listener thread.
 */
void UninitializeCommandListenerThread();

/**
 * @brief Register command.
 *
 * @param command An ADUC_Command information.
 * @return int If success, returns index of the registered command. Otherwise, returns -1.
 */
int RegisterCommand(ADUC_Command* command);

/**
 * @brief Unregister command.
 *
 * @param command Pointer to a command to unregister.
 * @return _Bool If success, return true. Otherwise, returns false.
 */
_Bool UnregisterCommand(ADUC_Command* command);

#endif /* ADUC_COMMAND_HELPER_H */
