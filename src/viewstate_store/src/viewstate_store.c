#include "aduc/viewstate_store.h"
#include "aduc/result.h"
#include "aduc/logging.h"
#include "aduc/types/adu_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <parson.h>

/**
 * @brief Check if the state store file exists
 * @param file_path The path to the state store file
 * @return true if file exists, false otherwise
 */
static bool does_statestore_file_exist(const char* file_path)
{
    if (file_path == NULL)
    {
        return false;
    }

    struct stat st;
    return (stat(file_path, &st) == 0);
}

/**
 * @brief Create the state store file and directories if they don't exist
 * @param file_path The path to the state store file
 * @param initial_state The initial service status to write
 * @return ADUC_Result indicating success or failure
 */
static ADUC_Result create_state_store(const char* file_path, ADUC_ServiceStatus initial_state)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };

    if (file_path == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Create directory structure if it doesn't exist
    char* dir_path = strdup(file_path);
    if (dir_path == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    char* last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL)
    {
        *last_slash = '\0';

        // Create directories recursively
        char temp_path[PATH_MAX];
        char* token;
        char* remaining = dir_path + 1; // Skip the leading '/'

        snprintf(temp_path, sizeof(temp_path), "/");

        while ((token = strtok(remaining, "/")) != NULL)
        {
            remaining = NULL; // For subsequent calls to strtok
            strncat(temp_path, token, sizeof(temp_path) - strlen(temp_path) - 1);

            struct stat st;
            if (stat(temp_path, &st) != 0)
            {
                if (mkdir(temp_path, 0755) != 0 && errno != EEXIST)
                {
                    Log_Error("Failed to create directory %s: %s", temp_path, strerror(errno));
                    free(dir_path);
                    result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
                    return result;
                }
            }
            strncat(temp_path, "/", sizeof(temp_path) - strlen(temp_path) - 1);
        }
    }

    free(dir_path);

    // Create the JSON content
    JSON_Value* root_value = json_value_init_object();
    if (root_value == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    JSON_Object* root_object = json_value_get_object(root_value);
    if (root_object == NULL)
    {
        json_value_free(root_value);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    if (json_object_set_number(root_object, "version", 1) != JSONSuccess ||
        json_object_set_number(root_object, "viewstate", (double)initial_state) != JSONSuccess)
    {
        json_value_free(root_value);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Write to file with advisory lock
    int fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        Log_Error("Failed to open state store file for writing: %s", strerror(errno));
        json_value_free(root_value);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Apply advisory lock
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; // Lock entire file

    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        Log_Error("Failed to acquire write lock on state store file: %s", strerror(errno));
        close(fd);
        json_value_free(root_value);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    char* json_string = json_serialize_to_string_pretty(root_value);
    if (json_string == NULL)
    {
        close(fd);
        json_value_free(root_value);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    ssize_t bytes_written = write(fd, json_string, strlen(json_string));
    json_free_serialized_string(json_string);
    json_value_free(root_value);

    // Release lock and close file
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);

    if (bytes_written == -1)
    {
        Log_Error("Failed to write to state store file: %s", strerror(errno));
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    result.ResultCode = ADUC_Result_Success;
    return result;
}

static ADUC_Result refresh_data_from_disk(ADUC_StateStoreData* data)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };
    JSON_Value* root_value = NULL;
    char* file_content = NULL;

    if (data == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    const char* file_path = ADUC_DATASTORE_FILE_PATH;

    // Check if file exists
    if (!does_statestore_file_exist(file_path))
    {
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_STORE_NOT_FOUND;
        return result;
    }

    // Open file for reading with advisory lock
    int fd = open(file_path, O_RDONLY);
    if (fd == -1)
    {
        Log_Error("Failed to open state store file for reading: %s", strerror(errno));
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Apply advisory read lock
    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; // Lock entire file

    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        Log_Error("Failed to acquire read lock on state store file: %s", strerror(errno));
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        Log_Error("Failed to get file stats: %s", strerror(errno));
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Read file content
    file_content = malloc(st.st_size + 1);
    if (file_content == NULL)
    {
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    ssize_t bytes_read = read(fd, file_content, st.st_size);

    // Release lock and close file
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);

    if (bytes_read != st.st_size)
    {
        Log_Error("Failed to read complete file content: %s", strerror(errno));
        free(file_content);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    file_content[bytes_read] = '\0';

    // Parse JSON
    root_value = json_parse_string(file_content);

    if (root_value == NULL)
    {
        Log_Error("Failed to parse JSON from state store file");
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    JSON_Object* root_object = json_value_get_object(root_value);
    if (root_object == NULL)
    {
        json_value_free(root_value);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Check version
    double version = json_object_get_number(root_object, "version");
    if (version != 1.0)
    {
        Log_Error("Invalid schema version in state store file: %f (expected 1)", version);
        json_value_free(root_value);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INVALID_SCHEMA_VERSION;
        return result;
    }

    // Read viewstate
    double viewstate_value = json_object_get_number(root_object, "viewstate");
    data->viewState = (ADUC_ServiceStatus)viewstate_value;

    result.ResultCode = ADUC_Result_Success;
done:
    free(file_content);
    json_value_free(root_value);
    if (fd > -1)
    {
        close(fd);
    }

    return result;
}

ADUC_StateStoreHandle ADUC_StateStore_Create()
{
    ADUC_StateStoreHandle handle = calloc(1, sizeof(ADUC_StateStoreData));
    memset(handle, 0, sizeof(ADUC_StateStoreData));
    ADUC_StateStoreData* data = (ADUC_StateStoreData*)handle;
    data->version = 1;
    return handle;
}

void ADUC_StateStore_Destroy(ADUC_StateStoreHandle h)
{
    memset(h, 0, sizeof(ADUC_StateStoreData));
    free(h);
}

ADUC_Result ADUC_StateStore_GetServiceStatus(ADUC_StateStoreHandle stateStore, ADUC_ServiceStatus* out_status)
{
    ADUC_Result result = {0};
    if (stateStore == NULL || out_status == NULL)
    {
        result.ExtendedResult = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return ADUC_Result_Failure;
    }

    ADUC_StateStoreData* data = (ADUC_StateStoreData*)stateStore;
    result = refresh_data_from_disk(data);
    if (result.ResultCode != ADUC_Result_Success)
    {
        return result;
    }
    *out_status = data->viewState;
    return ADUC_Result_Success;
}

ADUC_Result ADUC_StateStore_SetServiceStatus(ADUC_StateStoreHandle stateStore, ADUC_ServiceStatus status)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };

    if (stateStore == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    ADUC_StateStoreData* data = (ADUC_StateStoreData*)stateStore;
    const char* file_path = ADUC_DATASTORE_FILE_PATH;

    // Update in-memory state
    data->viewState = status;

    // If file doesn't exist, create it
    if (!does_statestore_file_exist(file_path))
    {
        return create_state_store(file_path, status);
    }

    // File exists, update it
    // Open file for reading and writing with advisory lock
    int fd = open(file_path, O_RDWR);
    if (fd == -1)
    {
        Log_Error("Failed to open state store file for updating: %s", strerror(errno));
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Apply advisory write lock
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; // Lock entire file

    if (fcntl(fd, F_SETLKW, &lock) == -1)
    {
        Log_Error("Failed to acquire write lock on state store file: %s", strerror(errno));
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        Log_Error("Failed to get file stats: %s", strerror(errno));
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Read current content
    char* file_content = malloc(st.st_size + 1);
    if (file_content == NULL)
    {
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    ssize_t bytes_read = read(fd, file_content, st.st_size);
    if (bytes_read != st.st_size)
    {
        Log_Error("Failed to read complete file content: %s", strerror(errno));
        free(file_content);
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    file_content[bytes_read] = '\0';

    // Parse JSON
    JSON_Value* root_value = json_parse_string(file_content);
    free(file_content);

    if (root_value == NULL)
    {
        Log_Error("Failed to parse JSON from state store file");
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    JSON_Object* root_object = json_value_get_object(root_value);
    if (root_object == NULL)
    {
        json_value_free(root_value);
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Verify version (should be 1)
    double version = json_object_get_number(root_object, "version");
    if (version != 1.0)
    {
        Log_Error("Invalid schema version in state store file: %f (expected 1)", version);
        json_value_free(root_value);
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INVALID_SCHEMA_VERSION;
        return result;
    }

    // Update viewstate
    if (json_object_set_number(root_object, "viewstate", (double)status) != JSONSuccess)
    {
        json_value_free(root_value);
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Serialize updated JSON
    char* json_string = json_serialize_to_string_pretty(root_value);
    json_value_free(root_value);

    if (json_string == NULL)
    {
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    // Truncate and write updated content
    if (ftruncate(fd, 0) == -1 || lseek(fd, 0, SEEK_SET) == -1)
    {
        Log_Error("Failed to truncate/seek file: %s", strerror(errno));
        json_free_serialized_string(json_string);
        close(fd);
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    ssize_t bytes_written = write(fd, json_string, strlen(json_string));
    json_free_serialized_string(json_string);

    // Release lock and close file
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);

    if (bytes_written == -1)
    {
        Log_Error("Failed to write to state store file: %s", strerror(errno));
        result.ExtendedResultCode = ADUC_ERC_DATASTORE_VIEWSTATE_INTERNAL_ERROR;
        return result;
    }

    result.ResultCode = ADUC_Result_Success;
    return result;
}
