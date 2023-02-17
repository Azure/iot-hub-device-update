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

WimHandlerResultCode Install(const char* workFolder, const char* targetFile)
{
    WimHandlerResultCode result = WimHandlerResultCode::Install_UnknownException;

    UNREFERENCED_PARAMETER(workFolder);
    UNREFERENCED_PARAMETER(targetFile);

    return result;
}

WimHandlerResultCode Apply(const char* workFolder, const char* targetFile)
{
    WimHandlerResultCode result = WimHandlerResultCode::Apply_UnknownException;

    UNREFERENCED_PARAMETER(workFolder);
    UNREFERENCED_PARAMETER(targetFile);

    return result;
}

} // namespace WimStepHandler
