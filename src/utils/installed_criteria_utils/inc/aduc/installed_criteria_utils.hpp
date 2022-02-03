/**
 * @file installed_criteria_utils.hpp
 * @brief Contains utilities for managing Installed-Criteria data.
 *
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */
#ifndef ADUC_INSTALLED_CRITERIA_UTILS_HPP
#define ADUC_INSTALLED_CRITERIA_UTILS_HPP

#include "aduc/result.h"
#include <string>

/**
 * @brief Checks if the installed content matches the installed criteria.
 *
 * @param installedCriteria The installed criteria string. e.g. The firmware version or APT id.
 *  installedCriteria has already been checked to be non-empty before this call.
 *
 * @return ADUC_Result
 */
const ADUC_Result GetIsInstalled(const char* installedCriteriaFilePath, const std::string& installedCriteria);

/**
 * @brief Persist specified installedCriteria in a file and mark its state as 'installed'.
 *
 * @param installedCriteriaFilePath A full path to installed criteria data file.
 * @param installedCriteria An installed criteria string.
 *
 * @return bool A boolean indicates whether installedCriteria added successfully.
 */
const bool PersistInstalledCriteria(const char* installedCriteriaFilePath, const std::string& installedCriteria);

/**
 * @brief Remove specified installedCriteria from installcriteria data file.
 *
 * Note: it will remove duplicate installedCriteria entries.<br/>
 * For example, only the baz array element would remain after a call with "bar" installedCriteria:<br/>
 * \code
 * [
 *   {"installedCriteria": "bar", "state": "installed", "timestamp": "<time 1>"},
 *   {"installedCriteria": "bar", "state": "installed", "timestamp": "<time 2>"},
 *   {"installedCriteria": "baz", "state": "installed", "timestamp": "<time 3>"},
 * ]
 * \endcode
 *
 * @param installedCriteriaFilePath A full path to installed criteria data file.
 * @param installedCriteria An installed criteria string. Case-sensitive match is used.
 *
 * @return bool 'True' if the specified installedCriteria doesn't exist, file doesn't exist, or removed successfully.
 */
const bool RemoveInstalledCriteria(const char* installedCriteriaFilePath, const std::string& installedCriteria);

/**
 * @brief Remove all installed criteria data.
 *
 */
void RemoveAllInstalledCriteria();

#endif // ADUC_INSTALLED_CRITERIA_UTILS_HPP
