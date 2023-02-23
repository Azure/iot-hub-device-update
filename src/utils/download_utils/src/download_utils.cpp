/**
 * @file download_utils.cpp
 * @brief Implementation file for utils used by content downloader extensions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/download_utils.hpp"
#include <sstream>

namespace aduc
{
namespace download_utils
{
std::string build_download_filepath(const char* workFolder, const ADUC_FileEntity* entity)
{
    std::stringstream fullFilePath;
    fullFilePath << workFolder << "/" << entity->TargetFilename;
    return fullFilePath.str();
}

} // namespace download_utils
} // namespace aduc
