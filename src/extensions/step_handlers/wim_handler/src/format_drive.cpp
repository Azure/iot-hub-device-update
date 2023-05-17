#include "format_drive.hpp"
#include "com_helpers.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <WbemCli.h>

#define CHR(cmd)             \
    {                        \
        HRESULT hrTmp = cmd; \
        if (FAILED(hrTmp))   \
        {                    \
            return hrTmp;    \
        }                    \
    }

HRESULT FormatDrive(char driveLetter, const std::string& driveLabel)
{
    CComPtr<IWbemLocator> spLocator;
    CHR(CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spLocator)));

    CComPtr<IWbemServices> spService;
    CHR(spLocator->ConnectServer(CComBSTR(L"ROOT\\CIMV2"), NULL, NULL, NULL, 0, NULL, NULL, &spService));

    // Required, or we get E_ACCESSDENIED from IEnumWbemClassObject::Next
    CHR(CoSetProxyBlanket(
        spService,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE));

    // Enumerate all Win32_Volume instances looking for drive.

    CComPtr<IWbemClassObject> spClass;
    CHR(spService->GetObject(CComBSTR(L"Win32_Volume"), 0, NULL, &spClass, NULL));

    char szWQL[128];
    sprintf_s(szWQL, sizeof(szWQL), "SELECT * FROM Win32_Volume WHERE DriveLetter = '%c:'", driveLetter);

    CComPtr<IEnumWbemClassObject> spEnumerator;
    CHR(spService->ExecQuery(
        CComBSTR(L"WQL"),
        CComBSTR(szWQL),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &spEnumerator));
    if (spEnumerator == NULL)
    {
        return E_NOT_SET;
    }

    // Found the the drive, get the path to it.

    ULONG cItems;
    CComPtr<IWbemClassObject> spVolumeObj;

    CHR(spEnumerator->Next(WBEM_INFINITE, 1, &spVolumeObj, &cItems));
    if (cItems == 0)
    {
        return E_NOT_SET;
    }

    CComVariant varPath;
    CHR(spVolumeObj->Get(L"__PATH", 0, &varPath, NULL, NULL));

    // Get the Format method.

    CComBSTR methodName(L"Format");
    CComPtr<IWbemClassObject> spInParamsDefinition;
    CHR(spClass->GetMethod(methodName, 0, &spInParamsDefinition, NULL));
    CComPtr<IWbemClassObject> spClassInstance;
    CHR(spInParamsDefinition->SpawnInstance(0, &spClassInstance));

    // Run Format method.

    CComVariant varFileSystem(L"NTFS");
    CHR(spClassInstance->Put(L"FileSystem", 0, &varFileSystem, 0));
    CComVariant varQuickFormat(true);
    CHR(spClassInstance->Put(L"QuickFormat", 0, &varQuickFormat, 0));
    CComVariant varLabel(driveLabel.c_str());
    CHR(spClassInstance->Put(L"Label", 0, &varLabel, 0));

    CComPtr<IWbemClassObject> spOutParams;
    CHR(spService->ExecMethod(varPath.bstrVal(), methodName, 0, NULL, spClassInstance, &spOutParams, NULL));

    // Get result.

    CComVariant varReturnValue;
    CHR(spOutParams->Get(L"ReturnValue", 0, &varReturnValue, NULL, 0));

    int formatResult = varReturnValue.intVal();
    if (formatResult != 0)
    {
        return HRESULT_FROM_WIN32(formatResult);
    }

    return S_OK;
}
