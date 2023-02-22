#include "bcdedit.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <aduc/process_utils.hpp>

struct BcdEditResult
{
    int exitCode;
    std::vector<std::string> output;
};

static BcdEditResult LaunchBcdEdit(std::vector<std::string> args)
{
    BcdEditResult result;
    result.exitCode = ADUC_LaunchChildProcess("bcdedit.exe", args, result.output);
    return result;
}

static bool ConfigureBcdEntryAsActive(char driveLetter, const char* guid)
{
    BcdEditResult result;

    std::string partitionId("partition=");
    partitionId += driveLetter;
    partitionId += ":";

    std::string quotedIdentifier(1, '"');
    quotedIdentifier += guid;
    quotedIdentifier += '"';

    // bcdedit /set $guid device partition=D:

    result = LaunchBcdEdit(std::vector<std::string>{ "/set", quotedIdentifier, "device", partitionId });
    if (result.exitCode != 0)
    {
        return false;
    }

    // bcdedit /set $guid osdevice partition=D:

    result = LaunchBcdEdit(std::vector<std::string>{ "/set", quotedIdentifier, "osdevice", partitionId });
    if (result.exitCode != 0)
    {
        return false;
    }

    // bcdedit /set $guid path \windows\system32\winload.efi

    result = LaunchBcdEdit(
        std::vector<std::string>{ "/set", quotedIdentifier, "path", "\\windows\\system32\\winload.efi" });
    if (result.exitCode != 0)
    {
        return false;
    }

    // bcdedit /set $guid systemroot \Windows

    result = LaunchBcdEdit(std::vector<std::string>{ "/set", quotedIdentifier, "systemroot", "\\windows" });
    if (result.exitCode != 0)
    {
        return false;
    }

    // bcdedit /displayorder '{default}' $guid

    result = LaunchBcdEdit(std::vector<std::string>{ "/displayorder", "{default}", quotedIdentifier });
    if (result.exitCode != 0)
    {
        return false;
    }

    // bcdedit /default $guid

    result = LaunchBcdEdit(std::vector<std::string>{ "/default", quotedIdentifier });
    if (result.exitCode != 0)
    {
        return false;
    }

    return true;
}

bool ConfigureBCD(char driveLetter, const char* identifier)
{
    BcdEditResult result;

    // Enumerate all osloaders.  Check if partition=D: exists.

    result = LaunchBcdEdit(std::vector<std::string>{ "/enum", "osloader" });
    if (result.exitCode != 0)
    {
        return false;
    }

    std::string partitionId("partition=");
    partitionId += driveLetter;
    partitionId += ":";

    // Search for string containing "partition=D:"

    std::vector<std::string>::iterator it =
        std::find_if(result.output.begin(), result.output.end(), [&partitionId](const std::string& item) {
            return item.find(partitionId) != std::string::npos;
        });
    if (it != result.output.end())
    {
        // Found. Assume it's configured properly.
        return true;
    }

    // Not found, create a new entry and get its GUID.

    std::string quotedIdentifier(1, '"');
    quotedIdentifier += identifier;
    quotedIdentifier += '"';
    result = LaunchBcdEdit(std::vector<std::string>{ "/create", "/d", quotedIdentifier, "/application", "osloader" });
    if (result.exitCode != 0)
    {
        return false;
    }

    std::string guid;

    it = std::find_if(result.output.begin(), result.output.end(), [](const std::string& item) {
        return item.find("} was successfully created") != std::string::npos;
    });
    if (it != result.output.end())
    {
        guid = it->substr(10, 38);
    }

    if (guid.empty())
    {
        // Can't find GUID in output.
        return false;
    }

    if (!ConfigureBcdEntryAsActive(driveLetter, guid.c_str()))
    {
        return false;
    }

    // bcdedit /timeout 5

    // Ignore result, not critical.
    result = LaunchBcdEdit(std::vector<std::string>{ "/timeout", "5" });

    return true;
}
