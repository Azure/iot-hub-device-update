/**
 * @file process_utils.hpp
 * @brief Contains utilities for managing child processes.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <aduc/c_utils.h>
#include <aduc/logging.h>
#include <aduc/string_utils.hpp>

#include <iostream>
#include <sstream>
#include <string>

#include <chrono>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @brief Runs specified command in a new process and captures output, error messages, and exit code.
 *        The captured output and error messages will be written to ADUC_LOG_FILE.
 * 
 * @param comman Name of a command to run. If command doesn't contain '/', this function will 
 *               search for the specified command in PATH.
 * @param args List of arguments for the command.
 * @param output A standard output from the command.
 *  
 * @return An exit code from the command.
 */
int ADUC_LaunchChildProcess(const std::string& command, std::vector<std::string> args, std::string& output)
{
#define READ_END 0
#define WRITE_END 1

    int filedes[2];
    const int ret = pipe(filedes);
    if (ret != 0)
    {
        Log_Error("Cannot create output and error pipes. %s (errno %d).", strerror(errno), errno);
        return ret;
    }

    const int pid = fork();

    if (pid == 0)
    {
        // Running inside child process.

        // Redirect stdout and stderr to WRITE_END
        dup2(filedes[WRITE_END], STDOUT_FILENO);
        dup2(filedes[WRITE_END], STDERR_FILENO);

        close(filedes[READ_END]);
        close(filedes[WRITE_END]);

        std::vector<char*> argv;
        argv.reserve(args.size() + 2);
        argv.emplace_back(const_cast<char*>(command.c_str()));
        for (const std::string& arg : args)
        {
            argv.emplace_back(const_cast<char*>(arg.c_str()));
        }
        argv.emplace_back(nullptr);

        // The exec() functions only return if an error has occurred.
        // The return value is -1, and errno is set to indicate the error.
        int ret = execvp(command.c_str(), &argv[0]);

        fprintf(stderr, "execvp failed, ret %d, error %d\n", ret, errno);

        return ret;
    }

    close(filedes[WRITE_END]);

    for (;;)
    {
        char buffer[1024];
        ssize_t count;
        count = read(filedes[READ_END], buffer, sizeof(buffer));

        if (count == -1)
        {
            Log_Error("Read failed, error %d", errno);
            break;
        }

        if (count == 0)
        {
            break;
        }

        buffer[count] = 0;
        output += buffer;
    }

    int wstatus;
    int childExitStatus;

    waitpid(pid, &wstatus, 0);

    // Get the child process exit code.
    if (WIFEXITED(wstatus))
    {
        // Child process terminated normally.
        // e.g. by calling exit() or _exit(), or by returning from main().
        childExitStatus = WEXITSTATUS(wstatus);
    }
    else if (WIFSIGNALED(wstatus))
    {
        // Child process terminated by a signal.

        // Get the number of the signal that caused the child process to terminate.
        childExitStatus = WTERMSIG(wstatus);
        Log_Info("Child process terminated, signal %d", childExitStatus);
    }
    else if (WCOREDUMP(wstatus))
    {
        // Child process produced a core dump
        childExitStatus = WCOREDUMP(wstatus);
        Log_Error("Child process terminated, core dump %d", childExitStatus);
    }
    else
    {
        childExitStatus = EXIT_FAILURE;
        // Child process terminated abnormally.
        Log_Error("Child process terminated abnormally.", childExitStatus);
    }

    close(filedes[READ_END]);

    return childExitStatus;
}