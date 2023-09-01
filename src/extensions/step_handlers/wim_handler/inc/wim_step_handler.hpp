#ifndef WIM_STEP_HANDLER_HPP
#define WIM_STEP_HANDLER_HPP

#include <string>
#include <vector>

#include <aduc/result.h> // ADUC_FACILITY, ADUC_CONTENT

// Equivalent of MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ADUC_CONTENT_HANDLER_EXTERNAL
#define ADUC_ERC_EUCH(value)                                         \
    ((ADUC_FACILITY_EXTENSION_UPDATE_CONTENT_HANDLER & 0xF) << 0x1C) \
        | ((ADUC_CONTENT_HANDLER_EXTERNAL & 0xFF) << 0x14) | ((value)&0xFFFFF)

namespace WimStepHandler
{

enum RC
{
    // Meta codes that are translated to specific ADUC_Result codes

    Success = 1,
    Success_Reboot_Required = 2,

    // Codes specific to this handler.

    General_CantGetWorkFolder = ADUC_ERC_EUCH(100),

    Manifest_CantGetFileEntity = ADUC_ERC_EUCH(200),
    Manifest_WrongFileCount = ADUC_ERC_EUCH(201),
    Manifest_UnsupportedUpdateVersion = ADUC_ERC_EUCH(202),
    Manifest_MissingInstalledCriteria = ADUC_ERC_EUCH(203),

    Download_UnknownException = ADUC_ERC_EUCH(300),

    Install_UnknownException = ADUC_ERC_EUCH(400),

    Apply_UnknownException = ADUC_ERC_EUCH(500),
    Apply_BcdEditFailure = ADUC_ERC_EUCH(501),
};

// Returns true if installed, false otherwise.
bool IsInstalled(const char* installedCriteria);

ADUC_Result_t Install(const char* workFolder, const char* targetFile);

ADUC_Result_t Apply(const char* workFolder, const char* targetFile);

}; // namespace WimStepHandler

#endif
