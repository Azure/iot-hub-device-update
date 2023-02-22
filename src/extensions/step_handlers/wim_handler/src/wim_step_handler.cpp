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

namespace WimStepHandler
{

bool IsInstalled(const char* installedCriteria)
{
    // A trivial install check. Likely production code would use something more robust.

    // Check local drive for kernel32.dll version.  We'll match the file version of that
    // against the installed criteria string.
    char systemFolder[MAX_PATH];
    if (GetSystemDirectory(systemFolder, ARRAYSIZE(systemFolder)) == 0)
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

    hr = FormatDrive(TARGET_DRIVE);
    if (FAILED(hr))
    {
        return MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ERRNO(hr);
    }

    // Apply imagefile to D:\

    std::string driveRoot(1, TARGET_DRIVE);
    driveRoot += ":\\";

    hr = ApplyImage(sourceFile.c_str(), driveRoot.c_str(), tempPath.c_str());
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
