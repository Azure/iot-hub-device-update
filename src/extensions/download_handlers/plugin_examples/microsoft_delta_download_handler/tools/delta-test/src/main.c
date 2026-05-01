/**
 * @file main.c
 * @brief Test tool for delta reconstruction using Microsoft Delta Download Handler
 *
 * Usage: adu-delta-test -s <source-file> -d <delta-file> -o <output-file> [-v]
 */

#include <aduc/c_utils.h>
#include <aduc/hash_utils.h>
#include <aduc/logging.h>
#include <aduc/microsoft_delta_download_handler.h>
#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <aduc/types/workflow.h>
#include <aduc/workflow_utils.h>

// SHAversion is provided by aduc/hash_utils.h (OpenSSL-backed). The legacy
// include of azure_c_shared_utility/sha.h was removed to comply with the SDL
// Approved Cryptographic Libraries policy. See docs/security/cryptography.md.
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void print_usage(const char* program_name)
{
    printf("Usage: %s -s <source-file> -d <delta-file> -o <output-file> [-v]\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -s, --source FILE    Source file path\n");
    printf("  -d, --delta FILE     Delta file path\n");
    printf("  -o, --output FILE    Output file path\n");
    printf("  -v, --verbose        Enable verbose output\n");
    printf("  -h, --help           Show this help message\n");
}

static bool file_exists(const char* path)
{
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

static off_t get_file_size(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return -1;
    }
    return st.st_size;
}

int main(int argc, char* argv[])
{
    const char* source_file = NULL;
    const char* delta_file = NULL;
    const char* output_file = NULL;
    bool verbose = false;

    static struct option long_options[] = {
        { "source", required_argument, 0, 's' },
        { "delta", required_argument, 0, 'd' },
        { "output", required_argument, 0, 'o' },
        { "verbose", no_argument, 0, 'v' },
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "s:d:o:vh", long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 's':
            source_file = optarg;
            break;
        case 'd':
            delta_file = optarg;
            break;
        case 'o':
            output_file = optarg;
            break;
        case 'v':
            verbose = true;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    // Validate arguments
    if (source_file == NULL || delta_file == NULL || output_file == NULL)
    {
        fprintf(stderr, "Error: Missing required arguments\n\n");
        print_usage(argv[0]);
        return 1;
    }

    // Check files exist
    if (!file_exists(source_file))
    {
        fprintf(stderr, "Error: Source file does not exist: %s\n", source_file);
        return 1;
    }

    if (!file_exists(delta_file))
    {
        fprintf(stderr, "Error: Delta file does not exist: %s\n", delta_file);
        return 1;
    }

    // Get file sizes
    off_t source_size = get_file_size(source_file);
    off_t delta_size = get_file_size(delta_file);

    if (verbose)
    {
        printf("Source file: %s (%ld bytes)\n", source_file, (long)source_size);
        printf("Delta file:  %s (%ld bytes)\n", delta_file, (long)delta_size);
        printf("Output file: %s\n", output_file);
    }

    // Compute delta hash
    char* deltaHashValue = NULL;
    if (!ADUC_HashUtils_GetFileHash(delta_file, SHA256, &deltaHashValue))
    {
        fprintf(stderr, "Error: Failed to compute delta file hash\n");
        return 1;
    }

    if (verbose)
    {
        printf("Delta hash: %s\n", deltaHashValue);
    }

    // Create minimal workflow handle
    ADUC_WorkflowHandle workflowHandle = NULL;
    if (!workflow_init("test-workflow", false, &workflowHandle))
    {
        fprintf(stderr, "Error: Failed to initialize workflow\n");
        free(deltaHashValue);
        return 1;
    }

    // Set source cache directory
    workflow_set_string_property(workflowHandle, "sourceUpdateCacheDir", "/var/lib/adu/sdc");

    // Create FileEntity with delta as a RelatedFile
    ADUC_FileEntity fileEntity = { 0 };

    // Allocate RelatedFile array
    fileEntity.RelatedFiles = (ADUC_RelatedFile*)calloc(1, sizeof(ADUC_RelatedFile));
    if (fileEntity.RelatedFiles == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate RelatedFile\n");
        workflow_free(workflowHandle);
        free(deltaHashValue);
        return 1;
    }
    fileEntity.RelatedFileCount = 1;

    // Set up RelatedFile[0] for the delta file
    ADUC_RelatedFile* deltaRelatedFile = &(fileEntity.RelatedFiles[0]);
    deltaRelatedFile->FileName = strdup(delta_file);
    deltaRelatedFile->SizeInBytes = (size_t)delta_size;

    // Allocate hash array
    deltaRelatedFile->Hash = (ADUC_Hash*)calloc(1, sizeof(ADUC_Hash));
    if (deltaRelatedFile->Hash == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate hash\n");
        free(deltaRelatedFile->FileName);
        free(fileEntity.RelatedFiles);
        workflow_free(workflowHandle);
        free(deltaHashValue);
        return 1;
    }
    deltaRelatedFile->HashCount = 1;

    // Set hash
    deltaRelatedFile->Hash[0].type = strdup("sha256");
    deltaRelatedFile->Hash[0].value = deltaHashValue; // Transfer ownership

    // Set TargetFilename (this is on the FileEntity itself, not RelatedFile)
    fileEntity.TargetFilename = strdup("output.swu");

    if (verbose)
    {
        printf("\nCalling MicrosoftDeltaDownloadHandler_ProcessUpdate()...\n");
    }

    // Call the handler
    ADUC_Result handlerResult = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        workflowHandle,
        &fileEntity,
        output_file,
        NULL); // Use default cache path

    // Check result
    bool success = (handlerResult.ResultCode == ADUC_Result_Download_Handler_SuccessSkipDownload);

    if (success)
    {
        printf("SUCCESS: Delta reconstruction completed\n");
        if (verbose)
        {
            off_t output_size = get_file_size(output_file);
            printf("Output file size: %ld bytes\n", (long)output_size);
        }
    }
    else
    {
        fprintf(stderr, "FAILED: Handler returned error code %d\n", handlerResult.ResultCode);
        if (handlerResult.ExtendedResultCode != 0)
        {
            fprintf(stderr, "Extended result code: 0x%08X\n", handlerResult.ExtendedResultCode);
        }
    }

    // Cleanup
    free(fileEntity.TargetFilename);
    free(deltaRelatedFile->Hash[0].type);
    // deltaHashValue already freed (transferred ownership to hash[0].value)
    free(deltaRelatedFile->Hash);
    free(deltaRelatedFile->FileName);
    free(fileEntity.RelatedFiles);
    workflow_free(workflowHandle);

    return success ? 0 : 1;
}
