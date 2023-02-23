
/**
 * @file download_utils.h
 * @brief Header file for utils used by content downloader extensions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef DOWNLOAD_UTILS_HPP
#define DOWNLOAD_UTILS_HPP

#include <aduc/types/update_content.h> // ADUC_FileEntity
#include <string>

namespace aduc
{
namespace download_utils
{
std::string build_download_filepath(const char* workFolder, const ADUC_FileEntity* entity);

} // namespace download_utils
} // namespace aduc

#endif // DOWNLOAD_UTILS_HPP
