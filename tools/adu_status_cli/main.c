/**
 * @file main.c
 * @brief CLI tool to query the ADU agent status via Local API.
 *
 * This is an end-to-end verification tool that demonstrates the
 * cross-platform SDK client connecting to the Local API server.
 *
 * Usage: adu-status [--json] [--endpoint <path>]
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/aducsdk.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void print_usage(const char* progName)
{
    fprintf(stderr, "Usage: %s [OPTIONS]\n\n", progName);
    fprintf(stderr, "Query the ADU agent's current status via Local API.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --json       Output in JSON format\n");
    fprintf(stderr, "  --help       Show this help message\n");
}

int main(int argc, char** argv)
{
    bool jsonOutput = false;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--json") == 0)
        {
            jsonOutput = true;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else
        {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    ADUC_ServiceStatus status = GetAduServiceStatus();

    if (jsonOutput)
    {
        printf("{\"status_code\":%d,\"status_string\":\"%s\",\"is_error\":%s}\n",
               (int)status,
               ADUC_ServiceStatusToString(status),
               (status >= 10000) ? "true" : "false");
    }
    else
    {
        if (status >= 10000)
        {
            fprintf(stderr, "Error: %s (code %d)\n", ADUC_ServiceStatusToString(status), (int)status);
            return 2;
        }
        else
        {
            printf("Agent Status: %s (%d)\n", ADUC_ServiceStatusToString(status), (int)status);
        }
    }

    return (status >= 10000) ? 2 : 0;
}
