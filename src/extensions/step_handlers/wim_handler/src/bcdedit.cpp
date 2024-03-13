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

static bool ConfigureBcdEntry(char driveLetter, const char* guid)
{
    BcdEditResult result;

    const std::string partitionId(std::string("partition=") + driveLetter + ":");

    const std::string quotedIdentifier(std::string("\"") + guid + "\"");

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

    return true;
}

// Returns false if bcd entry exists, but couldn't be removed.
// Returns true if bcd entry not found, or was removed successfully.
static bool RemoveExistingBcdEntry(char driveLetter)
{
    // Enumerate all osloaders.  Check if partition=D: exists.

    BcdEditResult result = LaunchBcdEdit(std::vector<std::string>{ "/enum", "osloader" });
    if (result.exitCode != 0)
    {
        return false;
    }

    // Search for string containing "partition=D:"

    const std::string partitionId(std::string("partition=") + driveLetter + ":");

    std::vector<std::string>::iterator it =
        std::find_if(result.output.begin(), result.output.end(), [&partitionId](const std::string& item) {
            return item.find(partitionId) != std::string::npos;
        });
    if (it == result.output.end())
    {
        // Entry not found.
        return true;
    }

    it = std::find_if(result.output.begin(), result.output.end(), [](const std::string& item) {
        // e.g. "identifier              {5dca3a86-a7ec-11ed-a586-00155da00106}"
        return item.find("identifier ") != std::string::npos;
    });
    if (it == result.output.end())
    {
        // Unable to determine GUID.
        return false;
    }

    const std::string guid = it->substr(24);

    // Found. Remove it - recreate it below to ensure it's valid.
    const std::string quotedIdentifier(std::string("\"") + guid + "\"");

    // bcdedit /delete "{5dca3a8e-a7ec-11ed-a586-00155da00106}" /cleanup

    result = LaunchBcdEdit(std::vector<std::string>{ "/delete", quotedIdentifier, "/cleanup" });
    if (result.exitCode != 0)
    {
        // Unable to delete. Not much else we can do.
        return false;
    }

    // Removed successfully.
    return true;
}

bool SetBcdEntryAsDefault(const char* guid)
{
    // bcdedit /displayorder '{default}' $guid

    const std::string quotedIdentifier(std::string("\"") + guid + "\"");

    BcdEditResult result = LaunchBcdEdit(std::vector<std::string>{ "/displayorder", "{default}", quotedIdentifier });
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
    if (!RemoveExistingBcdEntry(driveLetter))
    {
        // Unable to remove existing entry, Not much else we can do.
        return true;
    }

    // Create a new entry and get its GUID.

    std::string quotedIdentifier(std::string("\"") + identifier + "\"");
    BcdEditResult result =
        LaunchBcdEdit(std::vector<std::string>{ "/create", "/d", quotedIdentifier, "/application", "osloader" });
    if (result.exitCode != 0)
    {
        return false;
    }

    std::string guid;

    std::vector<std::string>::iterator it =
        std::find_if(result.output.begin(), result.output.end(), [](const std::string& item) {
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

    if (!ConfigureBcdEntry(driveLetter, guid.c_str()))
    {
        return false;
    }

    if (!SetBcdEntryAsDefault(guid.c_str()))
    {
        return false;
    }

    // bcdedit /timeout 5

    // Ignore result, not critical.
    (void)LaunchBcdEdit(std::vector<std::string>{ "/timeout", "5" });

    return true;
}
