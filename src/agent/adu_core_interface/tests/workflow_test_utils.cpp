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