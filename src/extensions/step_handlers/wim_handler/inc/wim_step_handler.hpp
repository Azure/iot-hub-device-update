#ifndef WIM_STEP_HANDLER_HPP
#define WIM_STEP_HANDLER_HPP

#include <string>
#include <vector>

#include <aduc/result.h> // ADUC_FACILITY, ADUC_CONTENT

// result.h has functions that can't be used in an enum.  Creating an equivalent macro.
#define MAKE_EXTERNAL_ERC(value)                                     \
    ((ADUC_FACILITY_EXTENSION_UPDATE_CONTENT_HANDLER & 0xF) << 0x1C) \
        | ((ADUC_CONTENT_HANDLER_EXTERNAL & 0xFF) << 0x14) | ((value)&0xFFFFF)

namespace WimStepHandler
{

// Result Codes that can be returned from WimStepHandler implementation.
// Avoiding "enum class" to allow for implicit conversion to ADUC_Result_t
enum RC
{
    // Codes that are translated to specific ADUC_Result codes

    Success = 1,
    Success_Reboot_Required = 2,

    // Codes specific to this handler.

    General_CantGetWorkFolder = MAKE_EXTERNAL_ERC(100),

    Manifest_CantGetFileEntity = MAKE_EXTERNAL_ERC(200),
    Manifest_WrongFileCount = MAKE_EXTERNAL_ERC(201),
    Manifest_UnsupportedUpdateVersion = MAKE_EXTERNAL_ERC(202),
    Manifest_MissingInstalledCriteria = MAKE_EXTERNAL_ERC(203),

    Download_UnknownException = MAKE_EXTERNAL_ERC(300),

    Install_UnknownException = MAKE_EXTERNAL_ERC(400),

    Apply_UnknownException = MAKE_EXTERNAL_ERC(500),
};

// Returns true if installed, false otherwise.
bool IsInstalled(const char* installedCriteria);

RC Install(const char* workFolder, const char* targetFile);

RC Apply(const char* workFolder, const char* targetFile);

}; // namespace WimStepHandler

#endif
