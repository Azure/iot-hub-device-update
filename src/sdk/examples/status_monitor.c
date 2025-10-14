/**
 * @file status_monitor.c
 * @brief Continuous monitoring of ADU agent status with logging
 *
 * This example demonstrates continuous monitoring of the ADU agent status
 * with configurable intervals, logging options, and output formats.
 * Useful for system monitoring and debugging.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/aducsdk.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_INTERVAL 1
#define MAX_LOG_ENTRIES 1000

typedef enum
{
    FORMAT_HUMAN,
    FORMAT_JSON,
    FORMAT_CSV
} output_format_t;

static volatile int g_running = 1;
static int g_interval = DEFAULT_INTERVAL;
static output_format_t g_format = FORMAT_HUMAN;
static int g_show_timestamps = 1;
static FILE* g_output_file = NULL;
static int g_log_changes_only = 0;

void signal_handler(int)
{
    g_running = 0;
}

void print_usage(const char* program_name)
{
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Monitor Azure Device Update agent status continuously\n\n");
    printf("Options:\n");
    printf("  -i, --interval SECONDS     Monitoring interval (default: %d)\n", DEFAULT_INTERVAL);
    printf("  -f, --format FORMAT        Output format: human|json|csv (default: human)\n");
    printf("  -o, --output FILE          Write output to file instead of stdout\n");
    printf("  -c, --changes-only         Only log when status changes\n");
    printf("  -T, --no-timestamps        Don't include timestamps\n");
    printf("  -h, --help                 Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s -i 5 -f json -o status.log\n", program_name);
    printf("  %s -c -f csv > status_changes.csv\n", program_name);
}

const char* get_timestamp(void)
{
    static char timestamp[32];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    return timestamp;
}

const char* get_iso_timestamp(void)
{
    static char timestamp[32];
    time_t now = time(NULL);
    struct tm* tm_info = gmtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    return timestamp;
}

void output_human(ADUC_ServiceStatus status, const char* statusStr)
{
    FILE* out = g_output_file ? g_output_file : stdout;

    if (g_show_timestamps)
    {
        fprintf(out, "[%s] ", get_timestamp());
    }

    fprintf(out, "Status: %s (%d)", statusStr, status);

    if (status >= ADUC_ServiceStatus_ERROR_UnsupportedApiVersion)
    {
        fprintf(out, " - ERROR");
    }
    else if (status == ADUC_ServiceStatus_Idle || status == ADUC_ServiceStatus_Paused)
    {
        fprintf(out, " - Safe for power management");
    }
    else if (status >= ADUC_ServiceStatus_Downloading && status <= ADUC_ServiceStatus_Reporting)
    {
        fprintf(out, " - Update in progress");
    }

    fprintf(out, "\n");
    fflush(out);
}

void output_json(ADUC_ServiceStatus status, const char* statusStr)
{
    FILE* out = g_output_file ? g_output_file : stdout;

    fprintf(out, "{");

    if (g_show_timestamps)
    {
        fprintf(out, "\"timestamp\":\"%s\",", get_iso_timestamp());
    }

    fprintf(
        out,
        "\"status\":\"%s\",\"code\":%d,\"is_error\":%s,\"safe_for_power_mgmt\":%s}",
        statusStr,
        status,
        (status >= ADUC_ServiceStatus_ERROR_UnsupportedApiVersion) ? "true" : "false",
        (status == ADUC_ServiceStatus_Idle || status == ADUC_ServiceStatus_Paused) ? "true" : "false");

    fprintf(out, "\n");
    fflush(out);
}

void output_csv(ADUC_ServiceStatus status, const char* statusStr, int print_header)
{
    FILE* out = g_output_file ? g_output_file : stdout;

    if (print_header)
    {
        if (g_show_timestamps)
        {
            fprintf(out, "timestamp,");
        }
        fprintf(out, "status,code,is_error,safe_for_power_mgmt\n");
    }

    if (g_show_timestamps)
    {
        fprintf(out, "%s,", get_iso_timestamp());
    }

    fprintf(
        out,
        "%s,%d,%s,%s\n",
        statusStr,
        status,
        (status >= ADUC_ServiceStatus_ERROR_UnsupportedApiVersion) ? "true" : "false",
        (status == ADUC_ServiceStatus_Idle || status == ADUC_ServiceStatus_Paused) ? "true" : "false");

    fflush(out);
}

void output_status(ADUC_ServiceStatus status, const char* statusStr, int print_header)
{
    switch (g_format)
    {
    case FORMAT_HUMAN: // (more)human readable
        output_human(status, statusStr);
        break;
    case FORMAT_JSON:
        output_json(status, statusStr);
        break;
    case FORMAT_CSV:
        output_csv(status, statusStr, print_header);
        break;
    }
}

int run_monitor(void)
{
    ADUC_ServiceStatus last_status = ADUC_ServiceStatus_ERROR_Unknown;
    int first_output = 1;
    int iteration = 0;

    if (g_format == FORMAT_HUMAN && (!g_output_file || g_output_file == stdout))
    {
        printf("ADU Agent Status Monitor\n");
        printf("========================\n");
        printf("Press Ctrl+C to stop monitoring\n\n");
    }

    while (g_running)
    {
        ADUC_ServiceStatus status = GetAduServiceStatus();
        const char* statusStr = ADUC_ServiceStatusToString(status);

        int should_output = 1;
        if (g_log_changes_only && status == last_status && !first_output)
        {
            should_output = 0;
        }

        if (should_output)
        {
            output_status(status, statusStr, first_output && g_format == FORMAT_CSV);
            first_output = 0;
        }

        last_status = status;
        iteration++;

        sleep(g_interval);
    }

    if (g_format == FORMAT_HUMAN && (!g_output_file || g_output_file == stdout))
    {
        printf("\nMonitoring stopped after %d iterations.\n", iteration);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    const char* output_filename = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interval") == 0)
        {
            if (++i >= argc)
            {
                fprintf(stderr, "Error: -i requires an argument\n");
                return 2;
            }
            g_interval = atoi(argv[i]);
            if (g_interval <= 0)
            {
                fprintf(stderr, "Error: Invalid interval\n");
                return 2;
            }
        }
        else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0)
        {
            if (++i >= argc)
            {
                fprintf(stderr, "Error: -f requires an argument\n");
                return 2;
            }
            if (strcmp(argv[i], "human") == 0)
            {
                g_format = FORMAT_HUMAN;
            }
            else if (strcmp(argv[i], "json") == 0)
            {
                g_format = FORMAT_JSON;
            }
            else if (strcmp(argv[i], "csv") == 0)
            {
                g_format = FORMAT_CSV;
            }
            else
            {
                fprintf(stderr, "Error: Invalid format '%s'\n", argv[i]);
                return 2;
            }
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0)
        {
            if (++i >= argc)
            {
                fprintf(stderr, "Error: -o requires an argument\n");
                return 2;
            }
            output_filename = argv[i];
        }
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--changes-only") == 0)
        {
            g_log_changes_only = 1;
        }
        else if (strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--no-timestamps") == 0)
        {
            g_show_timestamps = 0;
        }
        else
        {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 2;
        }
    }

    if (output_filename)
    {
        g_output_file = fopen(output_filename, "w");
        if (!g_output_file)
        {
            fprintf(stderr, "Error: Cannot open output file '%s': %s\n", output_filename, strerror(errno));
            return 1;
        }
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int result = run_monitor();

    if (g_output_file)
    {
        fclose(g_output_file);
    }

    return result;
}
