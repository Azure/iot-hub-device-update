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

static BSTR SysAllocStringA(const char* str)
{
    DWORD wsize = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    BSTR bstr = SysAllocStringLen(NULL, wsize);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, bstr, wsize);
    return bstr;
}

static std::string GetFileNameWithoutExtension(const std::string& filename)
{
    const size_t start = filename.find_last_of('.');
    return (start == std::string::npos) ? "" : filename.substr(0, start);
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

    // Note: ideally this would be installed to "%LocalAppData%\Programs\audacity" and version there would be checked.

    // e.g. %LocalAppData%\Programs\audacity\audacity-win-3.2.2-x64\Audacity.exe
    std::string exeString(R"(%LocalAppData%\Programs\audacity\audacity-win-)");
    exeString += installedCriteria;
    exeString += R"(-x64\Audacity.exe)";

    ExpandEnvironmentStringsA(exeString.c_str(), exePath, ARRAYSIZE(exePath));

    DWORD size = GetFileVersionInfoSize(exePath, 0 /*dwHandle*/);
    if (size != 0)
    {
        void* pbuf = malloc(size);
        if (GetFileVersionInfo(exePath, 0 /*dwHandle*/, size, pbuf))
        {
            VS_FIXEDFILEINFO* pffi;
            UINT buflen;

            if (VerQueryValue(pbuf, R"(\)", reinterpret_cast<void**>(&pffi), &buflen))
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

    Log_Info(HANDLER_LOG_ID "IsInstalled '%s' %s installed.", exeString.c_str(), isInstalled ? "is" : "is not");
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

    char destFolder[MAX_PATH];
    ExpandEnvironmentStringsA(R"(%LocalAppData%\Programs\audacity)", destFolder, ARRAYSIZE(destFolder));

    BSTR bstrDestFolder = SysAllocStringA(destFolder);

    // CopyTo requires the destination folder to exist.
    int ret = SHCreateDirectoryEx(nullptr /*hwnd*/, destFolder, nullptr /*psa*/);
    if (ret == ERROR_SUCCESS || ret == ERROR_ALREADY_EXISTS)
    {
        BSTR bstrZipFile = SysAllocStringA(zipfile.c_str());

        succeeded = Unzip(bstrZipFile, bstrDestFolder);

        SysFreeString(bstrZipFile);
        SysFreeString(bstrDestFolder);
    }

    Log_Info(HANDLER_LOG_ID "Install '%s'->'%s' %s", zipfile.c_str(), destFolder, succeeded ? "succeeded" : "failed");

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

    // Audacity will be extracted to a subfolder under this folder
    char destFolder[MAX_PATH];
    ExpandEnvironmentStringsA(R"(%LocalAppData%\Programs\audacity)", destFolder, ARRAYSIZE(destFolder));

    // Name of audacity ZIP file.

    // e.g. %LocalAppData%\Programs\audacity\audacity-win-3.2.3-x64
    std::string instFolder(destFolder);
    // Append e.g. "audacity-win-3.2.2-x64" as zip file contains that as a top-level folder.
    instFolder += "/";
    instFolder += GetFileNameWithoutExtension(fileList[0].c_str());

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
            std::string str(instFolder);
            str += R"(\Audacity.exe)";
            hr = psl->SetPath(str.c_str());
            hr = psl->SetIconLocation(str.c_str(), 0);
            hr = psl->SetDescription("Audacity");
            hr = psl->SetWorkingDirectory(instFolder.c_str());

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
