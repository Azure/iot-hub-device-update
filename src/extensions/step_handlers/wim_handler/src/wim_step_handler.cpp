#include "wim_step_handler.hpp"

#include "aducresult.hpp"
#include "bcdedit.hpp" // ConfigureBCD
#include "com_helpers.hpp"
#include "file_version.hpp" // GetFileVersion
#include "format_drive.hpp" // FormatDrive
#include "wimg.hpp" // ApplyImage

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm> // replace
#include <string>

#include "aduc/logging.h"

// TODO: This should be set in the update metadata.
static const char TARGET_DRIVE = 'd';
static const std::string TARGET_VOLUME_LABEL{ "IOT" };

static std::string make_preferred(const char* path, const char* file = nullptr)
{
    std::string preferred;

    if (path[0] == '/')
    {
        preferred = "c:";
    }
    preferred += path;

    if (file != nullptr)
    {
        preferred += "/";
        preferred += file;
    }

    std::replace(preferred.begin(), preferred.end(), '/', '\\');

    return preferred;
}

/**
 * @brief Whether file exists.
 *
 * @param filepath The file path.
 * @return true when file exists
 * @details throws an HRESULT when failing to get attributes of the file.
 */
static bool FileExists(const std::string& filepath)
{
    const DWORD dwAttrib = GetFileAttributes(filepath.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
    {
        DWORD dwLastErr = GetLastError();
        Log_Error("GetFileAttributes failed for file '%s', GLE 0x%08x", filepath.c_str(), dwLastErr);
        throw HRESULT_FROM_WIN32(dwLastErr);
    }

    return !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

/**
 * @brief Get the volume label for the drive.
 *
 * @param driveRoot The drive root, such as "D:\\"
 * @return std::string The volume label
 * @details throws an HRESULT on failure to get volume info.
 */
static std::string GetDriveLabel(const std::string& driveRoot)
{
    char volumeName[MAX_PATH + 1];
    if(!GetVolumeInformationA(
        driveRoot.c_str(),
        volumeName,
        sizeof(volumeName),
        nullptr, // lpVolumeSerialNumber
        nullptr, // lpMaxComponentLen
        nullptr, // lpFileSystemFlags
        nullptr, // lpFileSystemNameBuffer
        0)) // nFileSystemNameSize
    {
        DWORD dwLastErr = GetLastError();
        Log_Error("GetVolumeInformationA failed for driveRoot '%s', GLE 0x%08x", driveRoot.c_str(), dwLastErr);
        throw HRESULT_FROM_WIN32(dwLastErr);
    }

    return std::string{ volumeName };
}

namespace WimStepHandler
{

bool IsInstalled(const char* installedCriteria)
{
    // A trivial install check. Likely production code would use something more robust.

    // Check local drive for kernel32.dll version.  We'll match the file version of that
    // against the installed criteria string.
    char systemFolder[MAX_PATH];
    if (GetSystemDirectoryA(systemFolder, ARRAYSIZE(systemFolder)) == 0)
    {
        Log_Error("GetSystemDirectoryA failed for folder '%s'", systemFolder);
        return false;
    }

    std::string referenceFile{ systemFolder };
    referenceFile += "\\kernel32.dll";

    return GetFileVersion(referenceFile.c_str()) == installedCriteria;
}

ADUC_Result_t Install(const char* workFolder, const char* targetFile)
{
    HRESULT hr;
    try
    {
        CCoInitialize coinit;

        const std::string tempPath{ make_preferred(workFolder) };
        const std::string sourceFile{ make_preferred(workFolder, targetFile) };

        std::string driveRoot(1, TARGET_DRIVE);
        driveRoot += ":\\";

        Log_Debug("Check if '%s' exists", sourceFile.c_str());
        if (!FileExists(sourceFile))
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            Log_Error("FileExists failure, hr: 0x%08x", hr);
            goto done;
        }

        Log_Debug("Get drive label for drive '%s'", driveRoot.c_str());
        if (GetDriveLabel(driveRoot) != TARGET_VOLUME_LABEL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NO_VOLUME_ID);
            Log_Error("GetDriveLabel failure, hr: 0x%08x", hr);
            goto done;
        }

        Log_Debug("Format drive");
        hr = FormatDrive(TARGET_DRIVE, TARGET_VOLUME_LABEL);
        if (FAILED(hr))
        {
            Log_Error("FormatDrive failure, hr: 0x%08x", hr);
            goto done;
        }

        Log_Debug("Apply Image '%s' to drive label '%s'", sourceFile.c_str(), driveRoot.c_str());
        hr = ApplyImage(sourceFile.c_str(), driveRoot.c_str(), tempPath.c_str());
        if (FAILED(hr))
        {
            Log_Error("ApplyImage failure, hr: 0x%08x", hr);
            goto done;
        }
    }
    catch (HRESULT errorHResult)
    {
        hr = errorHResult;
        Log_Error("Caught HRESULT exception, hr: 0x%08x", hr);
        goto done;
    }
    catch (...)
    {
        hr = E_UNEXPECTED;
        Log_Error("Caught unknown exception, hr: 0x%08x", hr);
        goto done;
    }

done:
    if (FAILED(hr))
    {
        return MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ERRNO(hr);
    }

    return WimStepHandler::RC::Success;
}

ADUC_Result_t Apply(const char* workFolder, const char* targetFile)
{
    UNREFERENCED_PARAMETER(workFolder);
    UNREFERENCED_PARAMETER(targetFile);

    CCoInitialize coinit;

    // Check if osloader contains entry for partition D:
    // If not, add entry named "Windows IoT" and set as default.

    if (!ConfigureBCD(TARGET_DRIVE, "Windows IoT"))
    {
        Log_Error("ConfigureBCD of target drive '%c' failing with RC::Apply_BcdEditFailure", TARGET_DRIVE);
        return RC::Apply_BcdEditFailure;
    }

    Log_Info("ConfigureBCD of target drive '%c' succeeded", TARGET_DRIVE);
    return WimStepHandler::RC::Success_Reboot_Required;
}

} // namespace WimStepHandler
