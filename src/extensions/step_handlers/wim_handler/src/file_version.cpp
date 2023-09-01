#include "file_version.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <memory> // unique_ptr
#include <sstream> // stringstream
#include <string>

std::string GetFileVersion(const char* path)
{
    std::string version;

    const DWORD size = GetFileVersionInfoSize(path, 0 /*dwHandle*/);
    if (size != 0)
    {
        std::unique_ptr<BYTE> buffer(new BYTE[size]);
        if (GetFileVersionInfo(path, 0 /*dwHandle*/, size, buffer.get()))
        {
            VS_FIXEDFILEINFO* pffi;
            UINT buflen;

            if (VerQueryValue(buffer.get(), "\\", reinterpret_cast<void**>(&pffi), &buflen))
            {
                std::stringstream ss;

                ss << HIWORD(pffi->dwProductVersionMS) << "." << LOWORD(pffi->dwProductVersionMS) << "."
                   << HIWORD(pffi->dwProductVersionLS) << "." << LOWORD(pffi->dwProductVersionLS);
                version = ss.str();
            }
        }
    }

    return version;
}
