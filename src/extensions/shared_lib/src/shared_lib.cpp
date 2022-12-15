/**
 * @file shared_lib.cpp
 * @brief Implementation for utility class that wraps loading a dynamic shared library and allows ensuring/getting symbols.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/shared_lib.hpp"
#include "aduc/hash_utils.h"

#include <algorithm> // std::for_each

// Note: this requires ${CMAKE_DL_LIBS}
#include <aducpal/dlfcn.h> // dlopen, dlerror, dlsym, dlclose

#include <stdexcept> // std::runtime_error

namespace aduc
{
SharedLib::SharedLib(const std::string& libPath)
{
    ADUCPAL_dlerror(); // clear
    void* handle = ADUCPAL_dlopen(libPath.c_str(), RTLD_LAZY);
    if (nullptr == handle)
    {
        char* error = ADUCPAL_dlerror();
        throw std::runtime_error(error != nullptr ? error : "dlopen");
    }

    libHandle = handle;
}

SharedLib::~SharedLib()
{
    if (libHandle != nullptr)
    {
        ADUCPAL_dlclose(libHandle);
        libHandle = nullptr;
    }
}

void SharedLib::EnsureSymbols(std::vector<std::string> symbols) const
{
    std::for_each(symbols.begin(), symbols.end(), [this](const std::string& sym) { GetSymbol(sym); });
}

void* SharedLib::GetSymbol(const std::string& symbol) const
{
    ADUCPAL_dlerror(); // clear
    void* sym = ADUCPAL_dlsym(libHandle, symbol.c_str());
    if (sym == nullptr)
    {
        char* error = ADUCPAL_dlerror();
        throw std::runtime_error(error != nullptr ? error : "dlsym");
    }

    return sym;
}

} // namespace aduc
