/**
 * @file main.cpp
 * @brief rootkey_workflow tests main entry point.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
// https://github.com/catchorg/Catch2/blob/master/docs/own-main.md
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <ostream>

int main(int argc, char* argv[])
{
    int result;

    result = Catch::Session().run(argc, argv);
    if (result != 0)
    {
        std::cout << "Catch session failed, err=" << result << std::endl;
    }

    return result;
}
