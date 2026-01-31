/**
 * @file main.cpp
 * @brief rootkey_workflow tests main entry point.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
// https://github.com/catchorg/Catch2/blob/master/docs/own-main.md
#include <catch2/catch_session.hpp>

#include <iostream>

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
