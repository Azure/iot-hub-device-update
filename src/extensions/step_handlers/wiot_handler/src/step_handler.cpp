#include "step_handler.hpp"

namespace StepHandler
{

bool IsInstalled(const char* installedCriteria)
{
    // TODO(JeffMill): Check version against file version and return ADUC_Result_IsInstalled_Installed if same.
    // Log_Info("Not installed. v%s != %s", installedCriteria, version.c_str());
    return false;
}

bool Install(const char* filePath)
{
    // TODO(JeffMill): Install file
    return false;
}

bool Apply(const char* workFolder)
{
    // TODO(JeffMill): Apply from folder
    return false;
}

} // namespace StepHandler
