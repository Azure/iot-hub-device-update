/**
 * @file shared_lib.hpp
 * @brief header for aduc::shared_lib class.
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_SHARED_LIB
#define ADUC_SHARED_LIB

#include <string>
#include <vector>

namespace aduc
{
class SharedLib
{
public:
    SharedLib(const std::string& libPath);
    ~SharedLib();

    void EnsureSymbols(std::vector<std::string> symbols) const;
    void* GetSymbol(const std::string& symbol) const;

private:
    void* libHandle;
};

} // namespace aduc

#endif // ADUC_SHARED_LIB
