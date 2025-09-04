/**
 * @file command_helper.c
 * @brief A helper library for inter-agent commands support.
 *
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */

#include "aduc/command_helper.h"
#include "aduc/logging.h"
#include "aduc/permission_utils.h"

#include <errno.h>
#include <fcntl.h>
#include <grp.h> // getgrnm
#include <pthread.h> // pthread_*
#include <stdbool.h> // bool
#include <stdio.h> // getline
#include <stdlib.h> // free
#include <string.h> // strlen
#include <sys/stat.h> // mkfifo
#include <unistd.h> // sleep
#include <stdint.h> // uint32_t
#include <limits.h> // PATH_MAX
#include "aduc/string_c_utils.h" // ADUC_Safe_StrCopyN

// keep this last to avoid interfering with system headers
#include "aduc/aduc_banned.h"

#define MAX_COMMAND_ARRAY_SIZE 1 // !< For version 1.0, we're supporting only 1 command.
#define COMMAND_MAX_LEN 64 // !< Max command length including NULL (legacy)
#define COMMAND_BUFFER_INITIAL_SIZE 256 // !< Initial buffer size for dynamic commands
#define COMMAND_BUFFER_MAX_SIZE 65536 // !< Maximum buffer size (64KB)
#define DELAY_BETWEEN_FAILED_OPERATION_SECONDS 10 // !< delay allowed between failed operations
#define MAX_CMD_LEN 32 // !< Max command length excluding the terminal NULL char

/**
 * @brief Structure for parsing new command format
 */
typedef struct _tagParsedCommand
{
    char* command; /**< e.g., "GET_VERSION" */
    uint32_t version; /**< The Command protocol schema version */
    char* response_path; /**< Path to response FIFO */
} ParsedCommand;

static pthread_mutex_t g_commandQueueMutex = PTHREAD_MUTEX_INITIALIZER; // !< Static defintion for the mutex to be used for communciating with the command threads
static pthread_t g_commandListenerThread; // !<  Static handle for the listener thread for routing info back from the child process
static bool g_commandListenerThreadCreated = false; // !< Static boolean switch to tell if the listener thread has been created
static bool g_terminate_thread_request = false; // !< Static boolean switch to tell if the thread needs to be terminated

// fwd decl
static bool CheckIncomingRequestFifoSecurity(const char* fifoPath);

/**
 * @brief Callback for reprocessing updates as they come in
 * @param command the command to be reprocessed / executed
 * @param context the context to be used for calling back into / accessing ADUC member values
 * @returns true on success; false otherwise
*/
bool ADUC_OnReprocessUpdate(const char* command, void* context);

/**
 * @brief Read a null-terminated command from file descriptor
 * @param fd FIFO to read from
 * @param out_command Output parameter for allocated command string
 * @return Number of bytes read, or -1 on error
 */
static ssize_t ReadCompleteCommand(int fd, char** out_command)
{
    if (out_command == NULL)
    {
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        Log_Error("Failed to stat fd %d: %s", fd, strerror(errno));
        return -1;
    }

    if (!S_ISFIFO(st.st_mode))
    {
        Log_Error("File descriptor %d is not a FIFO", fd);
        return -1;
    }

    if (!CheckIncomingRequestFifoSecurity(fifoPath))
    {
        Log_Error("FIFO path '%s' did not pass security checks", fifoPath);
        return -1;
    }

    char* buffer = malloc(COMMAND_BUFFER_INITIAL_SIZE);
    if (buffer == NULL)
    {
        Log_Error("Failed to allocate initial command buffer");
        return -1;
    }

    size_t buffer_size = COMMAND_BUFFER_INITIAL_SIZE;
    size_t total_read = 0;
    ssize_t bytes_read;
    bool found_terminator = false;

    while (!found_terminator && total_read < COMMAND_BUFFER_MAX_SIZE)
    {
        if (total_read >= buffer_size - 1) // space for at least one more byte
        {
            size_t new_size = buffer_size * 2;
            if (new_size > COMMAND_BUFFER_MAX_SIZE)
            {
                new_size = COMMAND_BUFFER_MAX_SIZE;
            }
            
            char* new_buffer = realloc(buffer, new_size);
            if (new_buffer == NULL)
            {
                Log_Error("Failed to expand command buffer");
                free(buffer);
                return -1;
            }
            buffer = new_buffer;
            buffer_size = new_size;
        }

        pthread_mutex_lock(&g_commandQueueMutex);
        bytes_read = read(fd, buffer + total_read, 1);
        pthread_mutex_unlock(&g_commandQueueMutex);

        if (bytes_read < 0)
        {
            Log_Error("FIFO read: %d", errno);
            free(buffer);
            return -1;
        }
        else if (bytes_read == 0) // EOF
        {
            break;
        }

        if (buffer[total_read] == '\0')
        {
            found_terminator = true;
        }
        total_read += bytes_read;
    }

    if (!found_terminator)
    {
        Log_Error("No NUL term");
        free(buffer);
        return -1;
    }

    *out_command = buffer;
    return (ssize_t)total_read;
}

/**
 * @brief Parse command line into components
 * @param input Input command string
 * @param input_len Length of input string
 * @param result Output parsed command structure
 * @return true on success, false on failure
 */
static bool ParseCommandLine(const char* input, size_t input_len, ParsedCommand* result)
{
    if (input == NULL || result == NULL || input_len == 0)
    {
        return false;
    }

    memset(result, 0, sizeof(ParsedCommand));

    char* work_buffer = malloc(input_len + 1);
    if (work_buffer == NULL)
    {
        return false;
    }
    memcpy(work_buffer, input, input_len);
    work_buffer[input_len] = '\0';

    char* command_part = strtok(work_buffer, ":");
    char* version_part = strtok(NULL, ":");
    char* args_part = strtok(NULL, ":");
    char* response_part = strtok(NULL, ":");

    if (command_part == NULL || version_part == NULL || response_part == NULL)
    {
        free(work_buffer);
        return false;
    }

    result->command = malloc(strlen(command_part) + 1);
    if (result->command == NULL)
    {
        free(work_buffer);
        return false;
    }
    strcpy(result->command, command_part);

    char* endptr;
    long version_long = strtol(version_part, &endptr, 10);
    if (*endptr != '\0' || version_long < 0 || version_long > UINT32_MAX)
    {
        free(result->command);
        free(work_buffer);
        return false;
    }
    result->version = (uint32_t)version_long;

    result->response_path = malloc(strlen(response_part) + 1);
    if (result->response_path == NULL)
    {
        free(result->command);
        free(work_buffer);
        return false;
    }
    strcpy(result->response_path, response_part);

    free(work_buffer);
    return true;
}

/**
 * @brief Free parsed command structure
 * @param cmd Parsed command to free
 */
static void FreeParsedCommand(ParsedCommand* cmd)
{
    if (cmd != NULL)
    {
        free(cmd->command);
        free(cmd->response_path);
        memset(cmd, 0, sizeof(ParsedCommand));
    }
}

/**
 * @brief Validate command format
 * @param command Command string to validate
 * @param length Length of command string
 * @return true if valid, false otherwise
 */
static bool ValidateCommandFormat(const char* command, size_t length)
{
    if (command == NULL || length == 0)
    {
        return false;
    }

    // Check for null terminator
    if (command[length - 1] != '\0')
    {
        return false;
    }

    int colon_count = 0;
    for (size_t i = 0; i < length - 1; i++)
    {
        if (command[i] == ':')
        {
            colon_count++;
        }
    }

    return colon_count == 3;
}

/**
 * @brief Write response to FIFO
 * @param response_path Path to response FIFO
 * @param response_code Response code to write
 * @return true on success, false on failure
 */
static bool WriteResponse(const char* response_path, uint32_t response_code)
{
    if (response_path == NULL)
    {
        return false;
    }

    int fd = open(response_path, O_WRONLY | O_NONBLOCK);
    if (fd < 0)
    {
        Log_Error("Failed to open response FIFO: %s", response_path);
        return false;
    }

    pthread_mutex_lock(&g_commandQueueMutex);
    ssize_t bytes_written = write(fd, &response_code, sizeof(response_code));
    pthread_mutex_unlock(&g_commandQueueMutex);
    close(fd);

    if (bytes_written != sizeof(response_code))
    {
        Log_Error("Failed to write complete response");
        return false;
    }

    return true;
}

/**
 * @brief Handle GET_VERSION command
 * @param parsed_cmd Parsed command structure
 * @return true on success, false on failure
 */
static bool HandleGetVersionCommand(const ParsedCommand* parsed_cmd)
{
    if (parsed_cmd == NULL)
    {
        return false;
    }

    Log_Info("Processing GET_VERSION command, version %u", parsed_cmd->version);

    uint32_t sdk_version;
    if (parsed_cmd->version == 1)
    {
        sdk_version = ViewStateManager_GetSdkVersion();
    }
    else
    {
        // Unsupported version
        sdk_version = ADUC_ServiceStatus_ERROR_UnsupportedApiVersion;
    }

    return WriteResponse(parsed_cmd->response_path, sdk_version);
}

static ADUC_Command* g_commands[MAX_COMMAND_ARRAY_SIZE] = {}; // !< Static list of commands being exectued of MAX_COMMAND_ARRAY_SIZE

/**
 * @brief Register command.
 *
 * @param command An ADUC_Command information.
 * @return int If success, returns index of the registered command. Otherwise, returns -1.
 */
int RegisterCommand(ADUC_Command* command)
{
    pthread_mutex_lock(&g_commandQueueMutex);
    int res = -1;
    // Find an empty slot to register a new command.
    for (int i = 0; i < MAX_COMMAND_ARRAY_SIZE; i++)
    {
        if (g_commands[i] == NULL)
        {
            Log_Info("Command register at slot#%d", i);
            g_commands[i] = command;
            res = i;
            goto done;
        }
    }

    Log_Error("No space available for command.");
done:
    pthread_mutex_unlock(&g_commandQueueMutex);
    return res;
}

/**
 * @brief Unregister command.
 *
 * @param command Pointer to a command to unregister.
 * @return bool If success, return true. Otherwise, returns false.
 */
bool UnregisterCommand(ADUC_Command* command)
{
    bool res = false;
    pthread_mutex_lock(&g_commandQueueMutex);
    for (int i = 0; i < MAX_COMMAND_ARRAY_SIZE; i++)
    {
        if (g_commands[i] == command)
        {
            Log_Info("Unregister command from stop#%d", i);
            g_commands[i] = NULL;
            res = true;
            goto done;
        }
    }
    Log_Warn("Command not found.");

done:
    pthread_mutex_unlock(&g_commandQueueMutex);
    return res;
}

/**
 * @brief Create a FIFO named pipe file.
 *
 * @return bool Returns true if success.
 */
static bool TryCreateFIFOPipe()
{
    // Try to create file if doesn't exist.
    struct stat st;
    if (stat(ADUC_COMMANDS_FIFO_NAME, &st) == -1)
    {
        // Create FIFO pipe for commands.
        // Only write to pipe
        if (mkfifo(ADUC_COMMANDS_FIFO_NAME, S_IRGRP | S_IWGRP | S_IRUSR | S_IWUSR) != 0)
        {
            int error_no = errno;
            switch (error_no)
            {
            case EACCES:
                Log_Error("No permission");
                break;
            case EDQUOT:
                Log_Error("The user's quota of disk blocks or inodes on the filesystem has been exhausted.");
                break;

            case EEXIST:
                Log_Error("pathname already exists.");
                break;

            case ENAMETOOLONG:
                Log_Error("Path or file name is too long.");
                break;

            case ENOENT:
                Log_Error("A directory component in pathname does not exist. (%s)", ADUC_COMMANDS_FIFO_NAME);
                break;

            case ENOSPC:
                Log_Error("The directory or filesystem has no room for the new file.");
                break;

            case ENOTDIR:
                Log_Error("A component used as a directory in pathname is not, in fact, a directory.");
                break;

            case EROFS:
                Log_Error("Pathname refers to a read-only filesystem.");
                break;

            default:
                Log_Error("Cannot create named pipe. errno '%d'.", error_no);
                break;
            }
            return false;
        }
    }

    Log_Info("Command FIFO file created successfully.");
    return true;
}

/**
 * @brief Perform following security checks:
 *     - The FIFO pipe owners must be adu:adu.
 *     - The calling process' effective group must be 'root' or 'adu'.
 *
 * @return bool Returns true if all security checks pass.
 */
static bool SecurityChecks()
{
    if (!(PermissionUtils_CheckOwnership(ADUC_COMMANDS_FIFO_NAME, ADUC_FILE_USER, ADUC_FILE_GROUP)))
    {
        Log_Error("Security error: '%s' has invalid owners.", ADUC_COMMANDS_FIFO_NAME);
        return false;
    }

    // Verify current user
    struct group* grp = getgrnam(ADUC_FILE_GROUP);
    if (grp == NULL)
    {
        // Failed to get 'adu' group information, bail.
        Log_Error("Cannot get 'adu' group info.");
        return false;
    }

    gid_t gid = getegid();
    if (gid != 0 /* root */
        && gid != grp->gr_gid /* adu */)
    {
        return false;
    }

    return true;
}

static bool CheckIncomingRequestFifoSecurity(const char* fifoPath)
{
    if (!(PermissionUtils_CheckOwnership(fifoPath, ADUC_FILE_USER, ADUC_FILE_GROUP))) {
        Log_Error("Security error: '%s' has invalid owners.", fifoPath);
        return false;
    }
    return true;
}

static void do_sleep(int seconds, bool* should_cancel)
{
    if (seconds < 0) return;
    for (int i = 0; i < seconds; i++)
    {
        if (should_cancel && *should_cancel) break;
        sleep(1);
    }
}

/**
 * @brief Command listener thread with support for variable-length commands
 *
 * @return void*
 */
static void* ADUC_CommandListenerThread(void* unused)
{
    bool threadCreated = false;
    int fd = 0;

    (void)unused; // avoid unused parameter warning

    if (!TryCreateFIFOPipe() || !SecurityChecks())
    {
        goto done;
    }

    threadCreated = true;

    do
    {
        if (fd <= 0)
        {
            fd = open(ADUC_COMMANDS_FIFO_NAME, O_RDONLY);
            if (fd <= 0)
            {
                Log_Error("Cannot open '%s' for read.", ADUC_COMMANDS_FIFO_NAME);
                do_sleep(DELAY_BETWEEN_FAILED_OPERATION_SECONDS, &g_terminate_thread_request);
                continue;
            }
        }

        Log_Info("Wait for cmd...");
        
        char* cmdline = NULL;
        ssize_t readSize = ReadCompleteCommand(fd, &cmdline);

        if (readSize < 0) // error
        {
            Log_Warn("Read error (error:%d).", errno);
            close(fd);
            fd = -1;
            do_sleep(DELAY_BETWEEN_FAILED_OPERATION_SECONDS, &g_terminate_thread_request);
        }

        if (readSize == 0) // EOF, no more data
        {
            close(fd);
            fd = -1;
            continue;
        }

        if (!ValidateCommandFormat(cmdline, (size_t)readSize))
        {
            Log_Warn("Invalid command format received, length=%d", readSize);
            free(cmdline);
            continue;
        }

        // Try to parse new command format first
        ParsedCommand parsed_cmd;
        if (ParseCommandLine(cmdline, (size_t)readSize - 1, &parsed_cmd)) // -1 to exclude null terminator
        {
            Log_Info("Processing new format command: %s (version %u)", parsed_cmd.command, parsed_cmd.version);
            
            bool handled = false;
            if (strncmp(parsed_cmd.command, "GET_VERSION", MAX_CMD_LEN) == 0)
            {
                handled = HandleGetVersionCommand(&parsed_cmd);
            }
            else if (strncmp(parsed_cmd.command, "GET_STATE", MAX_CMD_LEN) == 0)
            {
                handled = HandleGetVersionCommand(&parsed_cmd);
            }
            else
            {
                Log_Warn("Unsupported new format command: %s", parsed_cmd.command);

                // Write error response
                WriteResponse(parsed_cmd.response_path, ERROR_UnsupportedApiVersion);

                handled = true; // We handled it by sending an error
            }
            
            FreeParsedCommand(&parsed_cmd);
            
            if (!handled)
            {
                Log_Error("Failed to handle command: %s", parsed_cmd.command);
            }
        }
        else
        {
            // Fall back to legacy command processing
            pthread_mutex_lock(&g_commandQueueMutex);
            const ADUC_Command* matchedCommand = NULL;
            for (int i = 0; i < MAX_COMMAND_ARRAY_SIZE; i++)
            {
                if (g_commands[i] != NULL)
                {
                    size_t commandTextLen = strlen(g_commands[i]->commandText);
                    if (readSize < commandTextLen)
                    {
                        continue;
                    }

                    if (strncmp(commandLine, g_commands[i]->commandText, commandTextLen) == 0)
                    {
                        matchedCommand = g_commands[i];
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&g_commandQueueMutex);

            if (matchedCommand == NULL)
            {
                Log_Warn("Unsupported legacy command received: '%s'", commandLine);
            }
            else
            {
                // Command matched.
                Log_Info("Executing legacy command handler function for '%s'", commandLine);
                if (!matchedCommand->callback(commandLine, NULL))
                {
                    Log_Error("Cannot execute a command handler for '%s'.", commandLine);
                }
            }
        }
        
        free(commandLine);
    } while (!g_terminate_thread_request);

done:
    close(fileDescriptor);
    if (!threadCreated)
    {
        Log_Error("Cannot start the command listener thread.");
    }
    return NULL;
}

/**
 * @brief Send specified @p command to the main Device Update agent process.
 *
 * @param command A command to send.
 *
 * @return bool Returns true if success.
 */
bool SendCommand(const char* command)
{
    bool success = false;
    const size_t cmdLen = strlen(command);
    int fd = -1;

    if (command == NULL || *command == '\0')
    {
        Log_Error("Command is null or empty.");
        goto done;
    }

    if (cmdLen > COMMAND_BUFFER_MAX_SIZE - 1)
    {
        Log_Error("Command is too long (max %d characters).", COMMAND_BUFFER_MAX_SIZE - 1);
        goto done;
    }

    // Check if the writer can access the pipe.
    if (!SecurityChecks())
    {
        goto done;
    }

    fd = open(ADUC_COMMANDS_FIFO_NAME, O_WRONLY);
    if (fd < 0)
    {
        Log_Error("Fail to open pipe.");
        goto done;
    }

    // Write command with null terminator
    ssize_t size = write(fd, command, cmdLen + 1); // +1 for null terminator
    if (size != (ssize_t)(cmdLen + 1))
    {
        Log_Error("Fail to send command.");
        goto done;
    }

    Log_Info("Command sent successfully.");
    success = true;
done:
    if (fd >= 0)
    {
        close(fd);
    }
    return success;
}

/**
 * @brief Initialize command listener thread.
 */
bool InitializeCommandListenerThread()
{
    if (g_commandListenerThreadCreated)
    {
        Log_Warn("Command listener thread already created.");
        return false;
    }

    Log_Info("Initializing command listener thread");

    if (pthread_create(&g_commandListenerThread, NULL, ADUC_CommandListenerThread, NULL) == 0)
    {
        g_commandListenerThreadCreated = true;
        return true;
    }

    return false;
}

/**
 * @brief Uninitialize command listener thread.
 */
void UninitializeCommandListenerThread()
{
    Log_Info("De-initializing command listener thread");
    g_terminate_thread_request = true;
}
