/**
 * @file workflow_test_utils.c
 * @brief The implementation of workflow test utilities.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "workflow_test_utils.h"

#include <fstream>
#include <sstream>

std::string slurpTextFile(const std::string& path)
{
    std::ifstream file_stream{path};
    std::stringstream buffer;
    buffer << file_stream.rdbuf();
    return buffer.str();
}
