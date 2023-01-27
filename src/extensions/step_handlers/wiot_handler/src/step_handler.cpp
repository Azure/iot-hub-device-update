// *** IMPORTANT **
// TODO(JeffMill): The code in this file is temporary demo code.  NOT FOR PRODUCTION.
// *** IMPORTANT **

// To uninstall (after demo):
// Remove-Item -Recurse -Force "$env:LocalAppData/Programs/audacity"
// Remove-Item -Force "$env:AppData/Microsoft/Windows/Start Menu/Programs/Audacity.lnk"

#include "step_handler.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <objbase.h>
#include <shellapi.h>
#include <shldisp.h>
#include <shlobj.h>
#include <shlobj_core.h>

#include <algorithm> // std::replace
#include <fstream>
#include <string>

#include "aduc/logging.h"
#include "aduc/system_utils.h" // RmDir_Recursive

// Prefix sent to logging for identification
#define HANDLER_LOG_ID "[microsoft/wiot:1] "

static BSTR SysAllocStringA(const char* str)
{
    DWORD wsize = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    BSTR bstr = SysAllocStringLen(NULL, wsize);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, bstr, wsize);
    return bstr;
}

bool Unzip(BSTR bstrZipFile, BSTR bstrDestFolder)
{
    bool success = false;

    bool fCoUninitializeNeeded = (CoInitialize(nullptr) == S_OK);

    IShellDispatch* pISD;
    if (CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, reinterpret_cast<void**>(&pISD))
        == S_OK)
    {
        VARIANT vZipFile;
        vZipFile.vt = VT_BSTR;
        vZipFile.bstrVal = bstrZipFile;

        Folder* pZipFile;
        pISD->NameSpace(vZipFile, &pZipFile);
        if (pZipFile != nullptr)
        {
            FolderItems* pFilesInside;
            pZipFile->Items(&pFilesInside);
            if (pFilesInside != nullptr)
            {
                VARIANT vDestFolder;
                vDestFolder.vt = VT_BSTR;
                vDestFolder.bstrVal = bstrDestFolder;

                Folder* pDestFolder;
                pISD->NameSpace(vDestFolder, &pDestFolder);
                if (pDestFolder != nullptr)
                {
                    IDispatch* pDispatch;
                    pFilesInside->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&pDispatch));

                    VARIANT vItem;
                    vItem.vt = VT_DISPATCH;
                    vItem.pdispVal = pDispatch;

                    VARIANT vOptions;
                    vOptions.vt = VT_I4;
                    vOptions.lVal = FOF_NO_UI;

                    // Copy pFolderItem to pDestFolder
                    success = (pDestFolder->CopyHere(vItem, vOptions) == S_OK);

                    pDispatch->Release();

                    // CopyHere spins off a thread. Give it a second.
                    Sleep(1000);

                    pDestFolder->Release();
                }

                pFilesInside->Release();
            }

            pZipFile->Release();
        }

        pISD->Release();
    }

    if (fCoUninitializeNeeded)
    {
        CoUninitialize();
    }

    return success;
}

namespace StepHandler
{

bool IsInstalled(const char* installedCriteria)
{
    bool isInstalled = false;

    char exePath[MAX_PATH];

    std::string exeReference(R"(%LocalAppData%\Programs\audacity\Audacity.exe)");

    ExpandEnvironmentStringsA(exeReference.c_str(), exePath, ARRAYSIZE(exePath));

    DWORD size = GetFileVersionInfoSize(exePath, 0 /*dwHandle*/);
    if (size != 0)
    {
        void* pbuf = malloc(size);
        if (GetFileVersionInfo(exePath, 0 /*dwHandle*/, size, pbuf))
        {
            VS_FIXEDFILEINFO* pffi;
            UINT buflen;

            if (VerQueryValue(pbuf, "\\", reinterpret_cast<void**>(&pffi), &buflen))
            {
                char version[100];
                sprintf_s(
                    version,
                    ARRAYSIZE(version),
                    "%d.%d.%d.%d",
                    HIWORD(pffi->dwProductVersionMS),
                    LOWORD(pffi->dwProductVersionMS),
                    HIWORD(pffi->dwProductVersionLS),
                    LOWORD(pffi->dwProductVersionLS));

                // Note: This is NOT how version checks work. Don't copy this!
                if (strcmp(version, installedCriteria) == 0)
                {
                    isInstalled = true;
                }
            }
        }

        free(pbuf);
    }

    Log_Info(HANDLER_LOG_ID "IsInstalled '%s' %s installed.", exePath, isInstalled ? "is" : "is not");
    return isInstalled;
}

bool Install(const char* workFolder, const std::vector<std::string>& fileList)
{
    bool succeeded = false;

    if (fileList.size() != 1)
    {
        // Currently only supporting one file.
        Log_Error(HANDLER_LOG_ID "Handler only supports one file.");
        return false;
    }

    // Shell APIs require full path to file and backslashes.

    std::string zipfile;
    if (workFolder[0] == '/')
    {
        zipfile = "c:";
    }
    zipfile += workFolder;
    zipfile += "/";
    zipfile += fileList[0];
    std::replace(zipfile.begin(), zipfile.end(), '/', '\\');

    //
    // Extract downloaded ZIP to %LocalAppData%\Programs\audacity
    //
    // Note: audacity zip contains a top level folder named e.g. "audacity-win-3.2.2-x64"
    // so extract to %LocalAppData%\Programs and rename that folder.
    //

    char programsFolder[MAX_PATH];
    ExpandEnvironmentStringsA(R"(%LocalAppData%\Programs)", programsFolder, ARRAYSIZE(programsFolder));

    BSTR bstrDestFolder = SysAllocStringA(programsFolder);

    // Unzip requires the destination folder to exist.
    int ret = SHCreateDirectoryEx(nullptr /*hwnd*/, programsFolder, nullptr /*psa*/);
    if (ret == ERROR_SUCCESS || ret == ERROR_ALREADY_EXISTS)
    {
        BSTR bstrZipFile = SysAllocStringA(zipfile.c_str());

        if (Unzip(bstrZipFile, bstrDestFolder))
        {
            std::string existingFolder(programsFolder);
            existingFolder += "\\audacity-win-*-x64";

            WIN32_FIND_DATA ffd;
            HANDLE hFind = FindFirstFile(existingFolder.c_str(), &ffd);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                // e.g. ...\Programs\audacity-win-3.2.2-x64
                existingFolder = programsFolder;
                existingFolder += '\\';
                existingFolder += ffd.cFileName;

                // ...\Programs\audacity
                std::string targetFolder(programsFolder);
                targetFolder += '\\';
                targetFolder += "audacity";

                // Remove existing folder
                ADUC_SystemUtils_RmDirRecursive(targetFolder.c_str());

                // Rename the folder that was created.
                succeeded = MoveFile(existingFolder.c_str(), targetFolder.c_str());

                FindClose(hFind);
            }
        }

        SysFreeString(bstrZipFile);
        SysFreeString(bstrDestFolder);
    }

    Log_Info(
        HANDLER_LOG_ID "Install '%s'->'%s' %s", zipfile.c_str(), programsFolder, succeeded ? "succeeded" : "failed");

    return succeeded;
}

bool Apply(const char* workFolder, const std::vector<std::string>& fileList)
{
    bool succeeded = false;

    UNREFERENCED_PARAMETER(workFolder);

    if (fileList.size() != 1)
    {
        // Currently only supporting one file.
        Log_Error(HANDLER_LOG_ID "Handler only supports one file.");
        return false;
    }

    //
    // Variables
    //

    char instFolder[MAX_PATH];
    ExpandEnvironmentStringsA(R"(%LocalAppData%\Programs\audacity)", instFolder, ARRAYSIZE(instFolder));

    // Name of audacity ZIP file.

    // Local audacity settings folder
    std::string settingsFolder(instFolder);
    // See https://manual.audacityteam.org/man/portable_audacity.html
    settingsFolder += R"(\Portable Settings)";

    // Start Menu shortcut file
    char shortcutFile[MAX_PATH];
    ExpandEnvironmentStringsA(
        R"(%AppData%\Microsoft\Windows\Start Menu\Programs\Audacity.lnk)", shortcutFile, ARRAYSIZE(shortcutFile));

    int ret = SHCreateDirectoryEx(nullptr /*hwnd*/, settingsFolder.c_str(), nullptr /*psa*/);
    if (ret == ERROR_SUCCESS || ret == ERROR_ALREADY_EXISTS)
    {
        //
        // Create default audacity.cfg file
        //

        std::string cfgFile(settingsFolder);
        cfgFile += R"(\audacity.cfg)";

        std::fstream file;
        file.open(cfgFile, std::ios_base::out);
        if (!file.is_open())
        {
            return false;
        }
        file << "PrefsVersion = 1.1.1r1\n";
        file << "WantAssociateFiles = 0\n";
        file << "[GUI]\n";
        file << "ShowSplashScreen = 0\n";
        file << "[Update]\n";
        file << "DefaultUpdatesChecking = 0\n";
        file.close();

        //
        // Create Windows Shell shortcut
        //

        IShellLink* psl;
        HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
        if (SUCCEEDED(hr))
        {
            std::string exePath(instFolder);
            exePath += "\\Audacity.exe";
            hr = psl->SetPath(exePath.c_str());
            hr = psl->SetIconLocation(exePath.c_str(), 0);
            hr = psl->SetDescription("Audacity");
            hr = psl->SetWorkingDirectory(instFolder);

            IPersistFile* pf;
            hr = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pf));
            if (SUCCEEDED(hr))
            {
                BSTR bstr = SysAllocStringA(shortcutFile);
                hr = pf->Save(bstr, TRUE);
                SysFreeString(bstr);

                pf->Release();

                succeeded = true;
            }
            psl->Release();
        }
    }

    Log_Info(HANDLER_LOG_ID "Apply %s", succeeded ? "succeeded" : "failed");

    return succeeded;
}

} // namespace StepHandler
