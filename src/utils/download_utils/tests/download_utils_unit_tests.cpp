
/**
 * @file download_utils_unit_tests.cpp
 * @brief Unit Tests for download_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/download_utils.hpp>
#include <aduc/types/update_content.h>
#include <catch2/catch.hpp>
#include <string>

using Catch::Matchers::Equals;

TEST_CASE("build_download_filepath")
{
    char filename[] = "disk.img.tgz";
    ADUC_FileEntity entity{};
    entity.TargetFilename = filename;
    const char* workfolder = "/path/to/sandbox";

    std::string filepath = aduc::download_utils::build_download_filepath(workfolder, &entity);

    CHECK_THAT(filepath.c_str(), Equals("/path/to/sandbox/disk.img.tgz"));
}
