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
#include "aducpal/unistd.h" // ADUCPAL_access

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
 * @brief Verifies that the file path exists.
 *
 * @param payloadFilePath The path to the file.
 * @return HRESULT S_OK when the file exists; an error hresult, otherwise.
 */
static HRESULT EnsurePayloadExists(const std::string& payloadFilePath)
{
    if (ADUCPAL_access(payloadFilePath.c_str(), F_OK) != 0)
    {
        int err;
        _get_errno(&err);
        return HRESULT_FROM_WIN32(err);
    }

    return S_OK;
}

/**
 * @brief Get the volume label for the drive
 *
 * @param[in] driveRoot The drive root path name, e.g. "C:\\" or "D:\\"
 * @param[out] outLabel The pointer to an existing std::string object to receive the resulting volume label.
 * @return HRESULT The value of GetLastError() as processed by HRESULT_FROM_WIN32 macro on error, or S_OK on success.
 * @details For failed returned HRESULT, the std::string object pointed to by outLabel will not be affected.
 */
static HRESULT GetDriveLabel(const std::string& driveRoot, std::string* outLabel)
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
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *outLabel = volumeName;
    return S_OK;
}

/**
 * @brief Verifies that the expected volume label exists on the given drive.
 *
 * @param driveRoot The drive root, such as "C:\\" or "D:\\".
 * @param expectedVolumeLabel The expected volume label.
 * @return HRESULT S_OK when the give volume label matches that of the drive; an error HRESULT, otherwise.
 */
static HRESULT EnsureExpectedDriveLabel(const std::string& driveRoot, const std::string& expectedVolumeLabel)
{
    std::string driveLabel;
    HRESULT hr = GetDriveLabel(driveRoot, &driveLabel);
    if (FAILED(hr))
    {
        return hr;
    }

    if (driveLabel != expectedVolumeLabel)
    {
        return HRESULT_FROM_WIN32(ERROR_NO_VOLUME_ID);
    }

    return S_OK;
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
        return false;
    }

    std::string referenceFile{ systemFolder };
    referenceFile += "\\kernel32.dll";

    return GetFileVersion(referenceFile.c_str()) == installedCriteria;
}

ADUC_Result_t Install(const char* workFolder, const char* targetFile)
{
    HRESULT hr;

    CCoInitialize coinit;

    const std::string tempPath{ make_preferred(workFolder) };
    const std::string sourceFile{ make_preferred(workFolder, targetFile) };

    std::string driveRoot(1, TARGET_DRIVE);
    driveRoot += ":\\";

    hr = EnsurePayloadExists(sourceFile);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = EnsureExpectedDriveLabel(driveRoot, TARGET_VOLUME_LABEL);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = FormatDrive(TARGET_DRIVE, TARGET_VOLUME_LABEL);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = ApplyImage(sourceFile.c_str(), driveRoot.c_str(), tempPath.c_str());
    if (FAILED(hr))
    {
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
        return RC::Apply_BcdEditFailure;
    }

    return WimStepHandler::RC::Success_Reboot_Required;
}

} // namespace WimStepHandler
