#include "wim_step_handler.hpp"
#include "aducresult.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "aduc/logging.h"

namespace WimStepHandler
{

bool IsInstalled(const char* installedCriteria)
{
    bool isInstalled = false;

    UNREFERENCED_PARAMETER(installedCriteria);

    return isInstalled;
}

RC Install(const char* workFolder, const char* targetFile)
{
    RC result = RC::Install_UnknownException;

    UNREFERENCED_PARAMETER(workFolder);
    UNREFERENCED_PARAMETER(targetFile);

    return result;
}

// Return Apply_Success_Reboot_Required to indicate reboot is required.
RC Apply(const char* workFolder, const char* targetFile)
{
    RC result = RC::Apply_UnknownException;

    UNREFERENCED_PARAMETER(workFolder);
    UNREFERENCED_PARAMETER(targetFile);

    return result;
}

} // namespace WimStepHandler
