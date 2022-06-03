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
static std::string& TrimLeading(std::string& s)
{
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), [](int ch) { return std::isspace(ch); }));
    return s;
}

// NOLINTNEXTLINE(google-runtime-references)
static std::string& TrimTrailing(std::string& s)
{
    s.erase(std::find_if_not(s.rbegin(), s.rend(), [](int ch) { return std::isspace(ch); }).base(), s.end());
    return s;
}

// NOLINTNEXTLINE(google-runtime-references)
static std::string& Trim(std::string& s)
{
    TrimLeading(s);
    TrimTrailing(s);
    return s;
}

/**
 * @brief Removes a char from the front and back of the string.
 * @param s The string
 * @param c The char to remove from front and back.
 * @return std::string& The reference to the possibly mutated input string.
 * @details If the string has the char at front and back, then input string will be mutated; else, it will be left untouched.
 */
static std::string& RemoveSurrounding(std::string& s, char c) // NOLINT(google-runtime-references)
{
    if (s.front() == c && s.back() == c)
    {
        s.erase(0, 1);
        s.pop_back();
    }

    return s;
}

static std::vector<std::string> Split(const std::string& str, const char separator)
{
    std::vector<std::string> tokens;
    std::istringstream stream(str);
    std::string token;
    while (std::getline(stream, token, separator))
    {
        tokens.emplace_back(token);
    }

    // getline ignores if the last character is separator
    // e.g. ":" only gives 1 vector count instead of 2
    // adding this to handle the corner case
    if (str.back() == separator)
    {
        tokens.emplace_back("");
    }
    return tokens;
}
} // namespace StringUtils
} // namespace ADUC

#endif // ADUC_STRING_UTILS_HPP
