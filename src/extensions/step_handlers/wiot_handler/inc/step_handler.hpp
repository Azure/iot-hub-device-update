#ifndef STEP_HANDLER_HPP
#define STEP_HANDLER_HPP

namespace StepHandler
{

bool IsInstalled(const char* installedCriteria);

bool Install(const char* filePath);

bool Apply(const char* workFolder);

}; // namespace StepHandler

#endif
