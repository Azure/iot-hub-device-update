#ifndef WIM_STEP_HANDLER_HPP
#define WIM_STEP_HANDLER_HPP

#include "wim_handler_result_code.hpp" // WimHandlerResultCode

#include <string>
#include <vector>

namespace WimStepHandler
{

// Returns true if installed, false otherwise.
bool IsInstalled(const char* installedCriteria);

WimHandlerResultCode Install(const char* workFolder, const char* targetFile);

WimHandlerResultCode Apply(const char* workFolder, const char* targetFile);

}; // namespace WimStepHandler

#endif
