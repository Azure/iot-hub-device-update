/**
 * @file simple_status_check.c
 * @brief Simple example showing how to check ADU agent status
 * @copyright Copyright (c) Microsoft Corporation. Licensed under the MIT License.
 */

#include <aduc/aducsdk.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char* program_name)
{
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Check Azure Device Update agent status\n\n");
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --verbose  Enable verbose output\n");
    printf("  -q, --quiet    Quiet mode (exit code only)\n");
    printf("\nExit codes:\n");
    printf("  0  Agent is operational (any non-error status)\n");
    printf("  1  Agent communication error\n");
    printf("  2  Invalid arguments\n");
}

const char* get_status_description(ADUC_ServiceStatus status)
{
    switch (status)
    {
    case ADUC_ServiceStatus_None:
        return "Agent is in initial state";
    case ADUC_ServiceStatus_Initializing:
        return "Agent is starting up and connecting to IoT Hub";
    case ADUC_ServiceStatus_Downloading:
        return "Agent is downloading update content";
    case ADUC_ServiceStatus_Installing:
        return "Agent is installing an update";
    case ADUC_ServiceStatus_Rebooting:
        return "System reboot is in progress";
    case ADUC_ServiceStatus_Reporting:
        return "Agent is reporting update results to IoT Hub";
    case ADUC_ServiceStatus_Paused:
        return "Agent is in quiet period before becoming idle";
    case ADUC_ServiceStatus_Idle:
        return "Agent is idle and ready for new updates";
    case ADUC_ServiceStatus_ERROR_UnsupportedApiVersion:
        return "SDK version is incompatible with agent";
    case ADUC_ServiceStatus_ERROR_AgentServiceNotRunning:
        return "ADU agent service is not running";
    case ADUC_ServiceStatus_ERROR_AgentServiceBrokenPipe:
        return "Communication pipe with agent is broken";
    case ADUC_ServiceStatus_ERROR_AgentServicePermission:
        return "Insufficient permissions to communicate with agent";
    case ADUC_ServiceStatus_ERROR_AgentServiceTimeout:
        return "Request to agent timed out";
    case ADUC_ServiceStatus_ERROR_AgentServiceInternal:
        return "Internal error in agent communication";
    case ADUC_ServiceStatus_ERROR_Unknown:
        return "Unknown error occurred";
    default:
        return "Unrecognized status code";
    }
}

int main(int argc, char* argv[])
{
    int verbose = 0;
    int quiet = 0;

    // Parse cmdline args
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            verbose = 1;
        }
        else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0)
        {
            quiet = 1;
        }
        else
        {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 2;
        }
    }

    if (!quiet)
    {
        printf("ADU Agent Status Check\n"
               "=====================\n");
        if (verbose)
        {
            printf("Querying ADU agent status...\n");
        }
    }

    ADUC_ServiceStatus status = GetAduServiceStatus();
    const char* statusStr = ADUC_ServiceStatusToString(status);
    const char* description = get_status_description(status);

    if (!quiet)
    {
        printf("Status: %s\n", statusStr);
        if (verbose)
        {
            printf("Code: %d\n", status);
            printf("Description: %s\n", description);
        }
    }

    if (status >= ADUC_ServiceStatus_ERROR_UnsupportedApiVersion)
    {
        if (!quiet)
        {
            fprintf(stderr, "Error: %s\n", description);
            if (verbose)
            {
                fprintf(stderr, "Troubleshooting tips:\n");
                switch (status)
                {
                case ADUC_ServiceStatus_ERROR_AgentServiceNotRunning:
                    fprintf(stderr, "  - Check if deviceupdate-agent service is running:\n");
                    fprintf(stderr, "    systemctl status deviceupdate-agent\n");
                    fprintf(stderr, "  - Start the service if needed:\n");
                    fprintf(stderr, "    sudo systemctl start deviceupdate-agent\n");
                    break;
                case ADUC_ServiceStatus_ERROR_AgentServicePermission:
                    fprintf(stderr, "  - Check file permissions on /var/lib/adu/api/\n");
                    fprintf(stderr, "  - Ensure your user has proper group membership\n");
                    break;
                case ADUC_ServiceStatus_ERROR_AgentServiceTimeout:
                    fprintf(stderr, "  - Agent may be overloaded or unresponsive\n");
                    fprintf(stderr, "  - Check agent logs: journalctl -u deviceupdate-agent\n");
                    break;
                default:
                    fprintf(stderr, "  - Check agent logs for more details\n");
                    break;
                }
            }
        }
        return 1;
    }
    else
    {
        if (!quiet && verbose)
        {
            printf("Agent communication successful.\n");
        }
        return 0;
    }
}
