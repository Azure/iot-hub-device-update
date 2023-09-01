#ifndef WIMG_HPP
#define WIMG_HPP

#define WIN32_LEAN_AND_MEAN
#include <objbase.h>

HRESULT ApplyImage(const char* source, const char* dest, const char* temp = nullptr);

#endif
