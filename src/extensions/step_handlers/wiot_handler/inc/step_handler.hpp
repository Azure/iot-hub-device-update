#ifndef STEP_HANDLER_HPP
#define STEP_HANDLER_HPP

#include <vector>
#include <string>

namespace StepHandler
{

bool IsInstalled(const char* installedCriteria);

bool Install(const char* workFolder, const std::vector<std::string>& fileList);

bool Apply(const char* workFolder, const std::vector<std::string>& fileList);

}; // namespace StepHandler

#endif
