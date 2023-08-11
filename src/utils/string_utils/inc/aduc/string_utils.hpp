/**
 * @file string_utils.hpp
 * @brief String utilities for C++ code.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_STRING_UTILS_HPP
#define ADUC_STRING_UTILS_HPP

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace ADUC
{
namespace StringUtils
{
// NOLINTNEXTLINE(google-runtime-references)
std::string& TrimLeading(std::string& s);

// NOLINTNEXTLINE(google-runtime-references)
std::string& TrimTrailing(std::string& s);

// NOLINTNEXTLINE(google-runtime-references)
std::string& Trim(std::string& s);

/**
 * @brief Removes a char from the front and back of the string.
 * @param s The string
 * @param c The char to remove from front and back.
 * @return std::string& The reference to the possibly mutated input string.
 * @details If the string has the char at front and back, then input string will be mutated; else, it will be left untouched.
 */
// NOLINTNEXTLINE(google-runtime-references)
std::string& RemoveSurrounding(std::string& s, char c);

std::vector<std::string> Split(const std::string& str, const char separator);

} // namespace StringUtils
} // namespace ADUC

#endif // ADUC_STRING_UTILS_HPP
