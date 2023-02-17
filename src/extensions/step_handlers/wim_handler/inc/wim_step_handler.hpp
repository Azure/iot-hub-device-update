#ifndef WIM_STEP_HANDLER_HPP
#define WIM_STEP_HANDLER_HPP

#include <string>
#include <vector>

namespace WimStepHandler
{

// Result Codes that can be returned from WimStepHandler implementation.
// Avoiding "enum class" to allow for implicit conversion to ADUC_Result_t
enum RC
{
    Success = 0,

    General_CantGetWorkFolder = 100,

    Manifest_CantGetFileEntity = 200,
    Manifest_WrongFileCount,
    Manifest_UnsupportedUpdateVersion,
    Manifest_MissingInstalledCriteria,

    Download_UnknownException = 300,

    Install_UnknownException = 400,

    Apply_UnknownException = 500,
    Apply_Success_Reboot_Required,
};

// Returns true if installed, false otherwise.
bool IsInstalled(const char* installedCriteria);

RC Install(const char* workFolder, const char* targetFile);

RC Apply(const char* workFolder, const char* targetFile);

}; // namespace WimStepHandler

#endif
