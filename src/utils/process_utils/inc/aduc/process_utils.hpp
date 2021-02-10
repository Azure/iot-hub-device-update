/**
 * @file process_utils.hpp
 * @brief Contains utilities for managing child processes.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_PROCESS_UTILS_HPP
#define ADUC_PROCESS_UTILS_HPP

#include <string>
#include <vector>

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
int ADUC_LaunchChildProcess(const std::string& command, std::vector<std::string> args, std::string& output);

#endif // ADUC_PROCESS_UTILS_HPP
