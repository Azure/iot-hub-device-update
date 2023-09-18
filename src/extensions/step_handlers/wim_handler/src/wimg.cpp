#include "wimg.hpp"

#include <codecvt>
#include <strsafe.h>
#include <wimgapi.h>

class WString
{
    WString(const WString&) = delete;
    WString& operator=(const WString&) = delete;

public:
    WString(const char* utf8str)
    {
        size_t utf8strLen;
        HRESULT hr = StringCchLengthA(utf8str, STRSAFE_MAX_CCH, &utf8strLen);

        if (hr != S_OK)
        {
            throw hr;
        }

        // Convert from utf8 -> wchar_t*
        int countWideChars = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, utf8str, (int)utf8strLen, nullptr, 0);

        if (countWideChars <= 0)
        {
            throw HRESULT_FROM_WIN32(GetLastError());
        }

        m_str.resize(countWideChars);

        countWideChars =
            MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, utf8str, (int)utf8strLen, m_str.data(), countWideChars);

        if (countWideChars <= 0)
        {
            throw HRESULT_FROM_WIN32(GetLastError());
        }
    }

    const wchar_t* c_str() const
    {
        return m_str.c_str();
    }

private:
    std::wstring m_str;
};

class WIMHandlePtr
{
public:
    explicit WIMHandlePtr(HANDLE handle) : m_handle(handle)
    {
    }

    WIMHandlePtr(const WIMHandlePtr&) = delete;
    WIMHandlePtr& operator=(const WIMHandlePtr&) = delete;

    ~WIMHandlePtr()
    {
        if (!is_null())
        {
            WIMCloseHandle(m_handle);
        }
    }

    HANDLE get() const
    {
        return m_handle;
    }

    bool is_null() const
    {
        return get() == nullptr;
    }

private:
    HANDLE m_handle;
};

static INT_PTR CALLBACK WIMOperationCallback(DWORD messageID, WPARAM wParam, LPARAM lParam, void* pvUserData)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(pvUserData);

    if (messageID == WIM_MSG_PROGRESS)
    {
        const UINT percent = static_cast<UINT>(wParam);
        if ((percent % 5) == 0)
        {
            // std::cout << "[WIM] " << percent << "%" << std::endl;
        }
    }

#if 0
    else if (messageID == WIM_MSG_QUERY_ABORT)
    {
        // return this if operation should be cancelled.
        return WIM_MSG_ABORT_IMAGE;
    }
#endif

    return WIM_MSG_SUCCESS;
}

HRESULT ApplyImage(const char* source, const char* dest, const char* temp /*= nullptr*/)
{
    WString wimPath{ source };
    WString applyPath{ dest };

    const DWORD access = WIM_GENERIC_READ | WIM_GENERIC_MOUNT;
    const DWORD mode = WIM_OPEN_EXISTING;
    const DWORD flags = 0;
    const DWORD comp = WIM_COMPRESS_NONE;

    WIMHandlePtr wimFile(WIMCreateFile(wimPath.c_str(), access, mode, flags, comp, nullptr));
    if (wimFile.is_null())
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (temp != nullptr)
    {
        WString tempPath{ temp };
        if (!WIMSetTemporaryPath(wimFile.get(), tempPath.c_str()))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (WIMRegisterMessageCallback(wimFile.get(), reinterpret_cast<FARPROC>(WIMOperationCallback), nullptr /*token*/)
        == INVALID_CALLBACK_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    const DWORD index = 1;
    WIMHandlePtr wimImage(WIMLoadImage(wimFile.get(), index));
    if (wimImage.is_null())
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // WIMApplyImage requires elevation.
    if (!WIMApplyImage(wimImage.get(), applyPath.c_str(), 0 /*flags*/))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    WIMUnregisterMessageCallback(wimImage.get(), reinterpret_cast<FARPROC>(WIMOperationCallback));

    return S_OK;
}
