#ifndef FORMAT_DRIVE_HPP
#define FORMAT_DRIVE_HPP

#define WIN32_LEAN_AND_MEAN
#include <objbase.h>
#include <string>

HRESULT FormatDrive(char driveLetter, const std::string& driveLabel);

#endif
