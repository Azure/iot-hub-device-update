/**
 * @file main.cpp
 * @brief process_utils unitest helper program.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <errno.h>
#include <getopt.h>
#include <sstream>
#include <stdio.h>

/**
 * @brief Main method.
 *
 * @param argc Count of arguments in @p argv.
 * @param argv Arguments, first is the process name.
 * @return int Return value. 0 for succeeded.
 *
 * @note argv[0]: process name
 * @note argv[1]: connection string.
 * @note argv[2..n]: optional parameters for upper layer.
 */
int main(int argc, char** argv)
{
    int exitStatus = EXIT_SUCCESS;
    int exitErrno = 0;

    while (exitStatus == 0)
    {
        // clang-format off
        static struct option long_options[] =
        {
            { "output-text",        required_argument,   nullptr, 'o' },
            { "error-text",         required_argument,   nullptr, 'e' },
            { "errno",              required_argument,   nullptr, 'n' },
            { "exit-status",        required_argument,   nullptr, 'x' },
            { "segfault",           no_argument,         nullptr, 's' },
            { nullptr, 0, nullptr, 0 }
        };
        // clang-format on

        /* getopt_long stores the option index here. */
        int option_index = 0;

        int option = getopt_long(argc, argv, "o:e:n:x:sd", long_options, &option_index);

        /* Detect the end of the options. */
        if (option == -1)
        {
            break;
        }

        switch (option)
        {
        case 'o':
            fprintf(stdout, "%s\n", optarg);
            break;

        case 'e':
            fprintf(stderr, "%s\n", optarg);
            break;

        case 'n':
        {
            char* endptr;
            errno = 0; /* To distinguish success/failure after call */
            exitErrno = strtol(optarg, &endptr, 10);
            if (errno != 0 || endptr == optarg)
            {
                printf("Invalid \errno %d\n", exitErrno);
            }
        }
        break;

        case 'x':
        {
            char* endptr;
            errno = 0; /* To distinguish success/failure after call */
            exitStatus = strtol(optarg, &endptr, 10);
            if (errno != 0 || endptr == optarg || exitStatus < 0 || exitStatus > 255)
            {
                printf("Invalid exit status %s (expecting 0-255).\n", optarg);
            }
        }
        break;

        case 's':
        {
            // NOLINTNEXTLINE(google-readability-casting,cppcoreguidelines-pro-type-cstyle-cast)
            auto foo = (char*)"hello world";
            for (int i = 20; i < 10000; i++)
            {
                *(foo + i) = 'a';
            }
            return -1;
        }
        break;

        default:
            printf("Unknown argument.");
            exitStatus = EXIT_FAILURE;
        }
    }

    printf("\nExiting with code %d, errno %d\nType echo $? to check.\n", exitStatus, exitErrno);
    if (exitErrno != 0)
    {
        errno = exitErrno;
    }

    return exitStatus;
}
